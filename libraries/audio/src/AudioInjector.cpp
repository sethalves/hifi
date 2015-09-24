//
//  AudioInjector.cpp
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>
#include <QtCore/QDataStream>

#include <NodeList.h>
#include <udt/PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AbstractAudioInterface.h"
#include "AudioRingBuffer.h"
#include "AudioLogging.h"
#include "SoundCache.h"
#include "AudioSRC.h"

#include "AudioInjector.h"

AudioInjector::AudioInjector(QObject* parent) :
    QObject(parent)
{

}

AudioInjector::AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions) :
    _audioData(sound->getByteArray()),
    _options(injectorOptions)
{
}

AudioInjector::AudioInjector(const QByteArray& audioData, const AudioInjectorOptions& injectorOptions) :
    _audioData(audioData),
    _options(injectorOptions)
{

}

void AudioInjector::setIsFinished(bool isFinished) {
    _isFinished = isFinished;
    // In all paths, regardless of isFinished argument. restart() passes false to prepare for new play, and injectToMixer() needs _shouldStop reset.
    _shouldStop = false;

    if (_isFinished) {
        emit finished();

        if (_localBuffer) {
            _localBuffer->stop();
            _localBuffer->deleteLater();
            _localBuffer = NULL;
        }

        _isStarted = false;

        if (_shouldDeleteAfterFinish) {
            // we've been asked to delete after finishing, trigger a queued deleteLater here
            QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
        }
    }
}

void AudioInjector::injectAudio() {
    if (!_isStarted) {
        _isStarted = true;
        // check if we need to offset the sound by some number of seconds
        if (_options.secondOffset > 0.0f) {

            // convert the offset into a number of bytes
            int byteOffset = (int) floorf(AudioConstants::SAMPLE_RATE * _options.secondOffset * (_options.stereo ? 2.0f : 1.0f));
            byteOffset *= sizeof(int16_t);

            _currentSendOffset = byteOffset;
        } else {
            _currentSendOffset = 0;
        }

        if (_options.localOnly) {
            injectLocally();
        } else {
            injectToMixer();
        }
    } else {
        qCDebug(audio) << "AudioInjector::injectAudio called but already started.";
    }
}

void AudioInjector::restart() {
    _isPlaying = true;
    connect(this, &AudioInjector::finished, this, &AudioInjector::restartPortionAfterFinished);
    if (!_isStarted || _isFinished) {
        emit finished();
    } else {
        stop();
    }
}
void AudioInjector::restartPortionAfterFinished() {
    disconnect(this, &AudioInjector::finished, this, &AudioInjector::restartPortionAfterFinished);
    setIsFinished(false);
    QMetaObject::invokeMethod(this, "injectAudio", Qt::QueuedConnection);
}

void AudioInjector::injectLocally() {
    bool success = false;
    if (_localAudioInterface) {
        if (_audioData.size() > 0) {

            _localBuffer = new AudioInjectorLocalBuffer(_audioData, this);

            _localBuffer->open(QIODevice::ReadOnly);
            _localBuffer->setShouldLoop(_options.loop);
            _localBuffer->setVolume(_options.volume);

            // give our current send position to the local buffer
            _localBuffer->setCurrentOffset(_currentSendOffset);

            success = _localAudioInterface->outputLocalInjector(_options.stereo, this);

            if (!success) {
                qCDebug(audio) << "AudioInjector::injectLocally could not output locally via _localAudioInterface";
            }
        } else {
            qCDebug(audio) << "AudioInjector::injectLocally called without any data in Sound QByteArray";
        }

    } else {
        qCDebug(audio) << "AudioInjector::injectLocally cannot inject locally with no local audio interface present.";
    }

    if (!success) {
        // we never started so we are finished, call our stop method
        stop();
    }

}

const uchar MAX_INJECTOR_VOLUME = 0xFF;

void AudioInjector::injectToMixer() {
    if (_currentSendOffset < 0 ||
        _currentSendOffset >= _audioData.size()) {
        _currentSendOffset = 0;
    }

    auto nodeList = DependencyManager::get<NodeList>();

    // make sure we actually have samples downloaded to inject
    if (_audioData.size()) {

        auto audioPacket = NLPacket::create(PacketType::InjectAudio);

        // setup the packet for injected audio
        QDataStream audioPacketStream(audioPacket.get());

        // pack some placeholder sequence number for now
        audioPacketStream << (quint16) 0;

        // pack stream identifier (a generated UUID)
        audioPacketStream << QUuid::createUuid();

        // pack the stereo/mono type of the stream
        audioPacketStream << _options.stereo;

        // pack the flag for loopback
        uchar loopbackFlag = (uchar) true;
        audioPacketStream << loopbackFlag;

        // pack the position for injected audio
        int positionOptionOffset = audioPacket->pos();
        audioPacketStream.writeRawData(reinterpret_cast<const char*>(&_options.position),
                                  sizeof(_options.position));

        // pack our orientation for injected audio
        audioPacketStream.writeRawData(reinterpret_cast<const char*>(&_options.orientation),
                                  sizeof(_options.orientation));

        // pack zero for radius
        float radius = 0;
        audioPacketStream << radius;

        // pack 255 for attenuation byte
        int volumeOptionOffset = audioPacket->pos();
        quint8 volume = MAX_INJECTOR_VOLUME * _options.volume;
        audioPacketStream << volume;

        audioPacketStream << _options.ignorePenumbra;

        int audioDataOffset = audioPacket->pos();

        QElapsedTimer timer;
        timer.start();
        int nextFrame = 0;

        bool shouldLoop = _options.loop;

        // loop to send off our audio in NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL byte chunks
        quint16 outgoingInjectedAudioSequenceNumber = 0;

        while (_currentSendOffset < _audioData.size() && !_shouldStop) {

            int bytesToCopy = std::min(((_options.stereo) ? 2 : 1) * AudioConstants::NETWORK_FRAME_BYTES_PER_CHANNEL,
                                       _audioData.size() - _currentSendOffset);

            //  Measure the loudness of this frame
            _loudness = 0.0f;
            for (int i = 0; i < bytesToCopy; i += sizeof(int16_t)) {
                _loudness += abs(*reinterpret_cast<int16_t*>(_audioData.data() + _currentSendOffset + i)) /
                (AudioConstants::MAX_SAMPLE_VALUE / 2.0f);
            }
            _loudness /= (float)(bytesToCopy / sizeof(int16_t));
            
            audioPacket->seek(0);
            
            // pack the sequence number
            audioPacket->writePrimitive(outgoingInjectedAudioSequenceNumber);
            
            audioPacket->seek(positionOptionOffset);
            audioPacket->writePrimitive(_options.position);
            audioPacket->writePrimitive(_options.orientation);

            volume = MAX_INJECTOR_VOLUME * _options.volume;
            audioPacket->seek(volumeOptionOffset);
            audioPacket->writePrimitive(volume);

            audioPacket->seek(audioDataOffset);

            // copy the next NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL bytes to the packet
            audioPacket->write(_audioData.data() + _currentSendOffset, bytesToCopy);

            // set the correct size used for this packet
            audioPacket->setPayloadSize(audioPacket->pos());

            // grab our audio mixer from the NodeList, if it exists
            SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
            
            if (audioMixer) {
                // send off this audio packet
                nodeList->sendUnreliablePacket(*audioPacket, *audioMixer);
                outgoingInjectedAudioSequenceNumber++;
            }

            _currentSendOffset += bytesToCopy;

            // send two packets before the first sleep so the mixer can start playback right away

            if (_currentSendOffset != bytesToCopy && _currentSendOffset < _audioData.size()) {

                // process events in case we have been told to stop and be deleted
                QCoreApplication::processEvents();

                if (_shouldStop) {
                    break;
                }

                // not the first packet and not done
                // sleep for the appropriate time
                int usecToSleep = (++nextFrame * AudioConstants::NETWORK_FRAME_USECS) - timer.nsecsElapsed() / 1000;

                if (usecToSleep > 0) {
                    usleep(usecToSleep);
                }
            }

            if (shouldLoop && _currentSendOffset >= _audioData.size()) {
                _currentSendOffset = 0;
            }
        }
    }

    setIsFinished(true);
    _isPlaying = !_isFinished; // Which can be false if a restart was requested
}

void AudioInjector::stop() {
    _shouldStop = true;

    if (_options.localOnly) {
        // we're only a local injector, so we can say we are finished right away too
        _isPlaying = false;
        setIsFinished(true);
    }
}

void AudioInjector::stopAndDeleteLater() {
    stop();
    QMetaObject::invokeMethod(this, "deleteLater", Qt::QueuedConnection);
}

AudioInjector* AudioInjector::playSound(const QString& soundUrl, const float volume, const float stretchFactor, const glm::vec3 position) {
    if (soundUrl.isEmpty()) {
        return NULL;
    }
    auto soundCache = DependencyManager::get<SoundCache>();
    if (soundCache.isNull()) {
        return NULL;
    }
    SharedSoundPointer sound = soundCache->getSound(QUrl(soundUrl));
    if (sound.isNull() || !sound->isReady()) {
        return NULL;
    }

    AudioInjectorOptions options;
    options.stereo = sound->isStereo();
    options.position = position;
    options.volume = volume;

    QByteArray samples = sound->getByteArray();
    if (stretchFactor == 1.0f) {
        return playSoundAndDelete(samples, options, NULL);
    }

    const int standardRate = AudioConstants::SAMPLE_RATE;
    const int resampledRate = standardRate * stretchFactor;
    const int channelCount = sound->isStereo() ? 2 : 1;

    AudioSRC resampler(standardRate, resampledRate, channelCount);

    const int nInputFrames = samples.size() / (channelCount * sizeof(int16_t));
    const int maxOutputFrames = resampler.getMaxOutput(nInputFrames);
    QByteArray resampled(maxOutputFrames * channelCount * sizeof(int16_t), '\0');

    int nOutputFrames = resampler.render(reinterpret_cast<const int16_t*>(samples.data()),
                                         reinterpret_cast<int16_t*>(resampled.data()),
                                         nInputFrames);

    Q_UNUSED(nOutputFrames);
    return playSoundAndDelete(resampled, options, NULL);
}

AudioInjector* AudioInjector::playSoundAndDelete(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface) {
    AudioInjector* sound = playSound(buffer, options, localInterface);
    sound->triggerDeleteAfterFinish();
    return sound;
}


AudioInjector* AudioInjector::playSound(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface) {
    QThread* injectorThread = new QThread();
    injectorThread->setObjectName("Audio Injector Thread");

    AudioInjector* injector = new AudioInjector(buffer, options);
    injector->_isPlaying = true;
    injector->setLocalAudioInterface(localInterface);

    injector->moveToThread(injectorThread);

    // start injecting when the injector thread starts
    connect(injectorThread, &QThread::started, injector, &AudioInjector::injectAudio);

    // connect the right slots and signals for AudioInjector and thread cleanup
    connect(injector, &AudioInjector::destroyed, injectorThread, &QThread::quit);
    connect(injectorThread, &QThread::finished, injectorThread, &QThread::deleteLater);

    injectorThread->start();
    return injector;
}
