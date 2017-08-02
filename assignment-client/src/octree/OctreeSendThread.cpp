//
//  OctreeSendThread.cpp
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <chrono>
#include <thread>

#include <NodeList.h>
#include <NumericalConstants.h>
#include <udt/PacketHeaders.h>
#include <PerfStat.h>

#include "OctreeQueryNode.h"
#include "OctreeSendThread.h"
#include "OctreeServer.h"
#include "OctreeServerConsts.h"
#include "OctreeLogging.h"

quint64 startSceneSleepTime = 0;
quint64 endSceneSleepTime = 0;

OctreeSendThread::OctreeSendThread(OctreeServer* myServer, const SharedNodePointer& node) :
    _myServer(myServer),
    _node(node),
    _nodeUuid(node->getUUID())
{
    QString safeServerName("Octree");

    // set our QThread object name so we can identify this thread while debugging
    setObjectName(QString("Octree Send Thread (%1)").arg(uuidStringWithoutCurlyBraces(_nodeUuid)));

    if (_myServer) {
        safeServerName = _myServer->getMyServerName();
    }

    qDebug() << qPrintable(safeServerName)  << "server [" << _myServer << "]: client connected "
                                            "- starting sending thread [" << this << "]";

    OctreeServer::clientConnected();
}

OctreeSendThread::~OctreeSendThread() {
    setIsShuttingDown();
    
    QString safeServerName("Octree");
    if (_myServer) {
        safeServerName = _myServer->getMyServerName();
    }

    qDebug() << qPrintable(safeServerName)  << "server [" << _myServer << "]: client disconnected "
                                            "- ending sending thread [" << this << "]";

    OctreeServer::clientDisconnected();
    OctreeServer::stopTrackingThread(this);
}

void OctreeSendThread::setIsShuttingDown() {
    _isShuttingDown = true;
}


bool OctreeSendThread::process() {
    if (_isShuttingDown) {
        return false; // exit early if we're shutting down
    }

    OctreeServer::didProcess(this);

    quint64  start = usecTimestampNow();

    // we'd better have a server at this point, or we're in trouble
    assert(_myServer);

    // don't do any send processing until the initial load of the octree is complete...
    if (_myServer->isInitialLoadComplete()) {
        if (auto node = _node.lock()) {
            OctreeQueryNode* nodeData = static_cast<OctreeQueryNode*>(node->getLinkedData());

            // Sometimes the node data has not yet been linked, in which case we can't really do anything
            if (nodeData && !nodeData->isShuttingDown()) {
                bool viewFrustumChanged = nodeData->updateCurrentViewFrustum();
                packetDistributor(node, nodeData, viewFrustumChanged);
            }
        } else {
            return false; // exit early if we're shutting down
        }
    }

    if (_isShuttingDown) {
        return false; // exit early if we're shutting down
    }

    // Only sleep if we're still running and we got the lock last time we tried, otherwise try to get the lock asap
    if (isStillRunning()) {
        // dynamically sleep until we need to fire off the next set of octree elements
        int elapsed = (usecTimestampNow() - start);
        int usecToSleep =  OCTREE_SEND_INTERVAL_USECS - elapsed;

        if (usecToSleep <= 0) {
            const int MIN_USEC_TO_SLEEP = 1;
            usecToSleep = MIN_USEC_TO_SLEEP;
        }

        {
            PerformanceWarning warn(false,"OctreeSendThread... usleep()",false,&_usleepTime,&_usleepCalls);
            std::this_thread::sleep_for(std::chrono::microseconds(usecToSleep));
        }

    }

    return isStillRunning();  // keep running till they terminate us
}

AtomicUIntStat OctreeSendThread::_usleepTime { 0 };
AtomicUIntStat OctreeSendThread::_usleepCalls { 0 };
AtomicUIntStat OctreeSendThread::_totalBytes { 0 };
AtomicUIntStat OctreeSendThread::_totalWastedBytes { 0 };
AtomicUIntStat OctreeSendThread::_totalPackets { 0 };

AtomicUIntStat OctreeSendThread::_totalSpecialBytes { 0 };
AtomicUIntStat OctreeSendThread::_totalSpecialPackets { 0 };


int OctreeSendThread::handlePacketSend(SharedNodePointer node, OctreeQueryNode* nodeData, bool dontSuppressDuplicate) {
    OctreeServer::didHandlePacketSend(this);

    // if we're shutting down, then exit early
    if (nodeData->isShuttingDown()) {
        return 0;
    }

    bool debug = _myServer->wantsDebugSending();
    quint64 now = usecTimestampNow();

    int numPackets = 0;

    // Here's where we check to see if this packet is a duplicate of the last packet. If it is, we will silently
    // obscure the packet and not send it. This allows the callers and upper level logic to not need to know about
    // this rate control savings.
    if (!dontSuppressDuplicate && nodeData->shouldSuppressDuplicatePacket()) {
        nodeData->resetOctreePacket(); // we still need to reset it though!
        return numPackets; // without sending...
    }

    // If we've got a stats message ready to send, then see if we can piggyback them together
    if (nodeData->stats.isReadyToSend() && !nodeData->isShuttingDown()) {
        // Send the stats message to the client
        NLPacket& statsPacket = nodeData->stats.getStatsMessage();

        // If the size of the stats message and the octree message will fit in a packet, then piggyback them
        if (nodeData->getPacket().getDataSize() <= statsPacket.bytesAvailableForWrite()) {

            // copy octree message to back of stats message
            statsPacket.write(nodeData->getPacket().getData(), nodeData->getPacket().getDataSize());

            int numBytes = statsPacket.getDataSize();
            _totalBytes += numBytes;
            _totalPackets++;
            // since a stats message is only included on end of scene, don't consider any of these bytes "wasted"
            // there was nothing else to send.
            int thisWastedBytes = 0;
            //_totalWastedBytes += 0;
            _trueBytesSent += numBytes;
            numPackets++;

            if (debug) {
                NLPacket& sentPacket = nodeData->getPacket();

                sentPacket.seek(sizeof(OCTREE_PACKET_FLAGS));

                OCTREE_PACKET_SEQUENCE sequence;
                sentPacket.readPrimitive(&sequence);

                OCTREE_PACKET_SENT_TIME timestamp;
                sentPacket.readPrimitive(&timestamp);

                qDebug() << "Adding stats to packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence <<
                        " timestamp: " << timestamp <<
                        " statsMessageLength: " << statsPacket.getDataSize() <<
                        " original size: " << nodeData->getPacket().getDataSize() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]";
            }

            // actually send it
            OctreeServer::didCallWriteDatagram(this);
            DependencyManager::get<NodeList>()->sendUnreliablePacket(statsPacket, *node);
        } else {
            // not enough room in the packet, send two packets

            // first packet
            OctreeServer::didCallWriteDatagram(this);
            DependencyManager::get<NodeList>()->sendUnreliablePacket(statsPacket, *node);

            int numBytes = statsPacket.getDataSize();
            _totalBytes += numBytes;
            _totalPackets++;
            // since a stats message is only included on end of scene, don't consider any of these bytes "wasted"
            // there was nothing else to send.
            int thisWastedBytes = 0;
            //_totalWastedBytes += 0;
            _trueBytesSent += numBytes;
            numPackets++;

            if (debug) {
                NLPacket& sentPacket = nodeData->getPacket();

                sentPacket.seek(sizeof(OCTREE_PACKET_FLAGS));

                OCTREE_PACKET_SEQUENCE sequence;
                sentPacket.readPrimitive(&sequence);

                OCTREE_PACKET_SENT_TIME timestamp;
                sentPacket.readPrimitive(&timestamp);

                qDebug() << "Sending separate stats packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence <<
                        " timestamp: " << timestamp <<
                        " size: " << statsPacket.getDataSize() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]";
            }

            // second packet
            OctreeServer::didCallWriteDatagram(this);
            DependencyManager::get<NodeList>()->sendUnreliablePacket(nodeData->getPacket(), *node);

            numBytes = nodeData->getPacket().getDataSize();
            _totalBytes += numBytes;
            _totalPackets++;
            // we count wasted bytes here because we were unable to fit the stats packet
            thisWastedBytes = udt::MAX_PACKET_SIZE - numBytes;
            _totalWastedBytes += thisWastedBytes;
            _trueBytesSent += numBytes;
            numPackets++;

            if (debug) {
                NLPacket& sentPacket = nodeData->getPacket();

                sentPacket.seek(sizeof(OCTREE_PACKET_FLAGS));

                OCTREE_PACKET_SEQUENCE sequence;
                sentPacket.readPrimitive(&sequence);

                OCTREE_PACKET_SENT_TIME timestamp;
                sentPacket.readPrimitive(&timestamp);

                qDebug() << "Sending packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence <<
                        " timestamp: " << timestamp <<
                        " size: " << nodeData->getPacket().getDataSize() << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]";
            }
        }
        nodeData->stats.markAsSent();
    } else {
        // If there's actually a packet waiting, then send it.
        if (nodeData->isPacketWaiting() && !nodeData->isShuttingDown()) {
            // just send the octree packet
            OctreeServer::didCallWriteDatagram(this);
            DependencyManager::get<NodeList>()->sendUnreliablePacket(nodeData->getPacket(), *node);

            int numBytes = nodeData->getPacket().getDataSize();
            _totalBytes += numBytes;
            _totalPackets++;
            int thisWastedBytes = udt::MAX_PACKET_SIZE - numBytes;
            _totalWastedBytes += thisWastedBytes;
            numPackets++;
            _trueBytesSent += numBytes;

            if (debug) {
                NLPacket& sentPacket = nodeData->getPacket();

                sentPacket.seek(sizeof(OCTREE_PACKET_FLAGS));

                OCTREE_PACKET_SEQUENCE sequence;
                sentPacket.readPrimitive(&sequence);

                OCTREE_PACKET_SENT_TIME timestamp;
                sentPacket.readPrimitive(&timestamp);

                qDebug() << "Sending packet at " << now << " [" << _totalPackets <<"]: sequence: " << sequence <<
                        " timestamp: " << timestamp <<
                        " size: " << numBytes << " [" << _totalBytes <<
                        "] wasted bytes:" << thisWastedBytes << " [" << _totalWastedBytes << "]";
            }
        }
    }

    // remember to track our stats
    if (numPackets > 0) {
        nodeData->stats.packetSent(nodeData->getPacket().getPayloadSize());
        nodeData->octreePacketSent();
        nodeData->resetOctreePacket();
    }

    _truePacketsSent += numPackets;
    return numPackets;
}

/// Version of octree element distributor that sends the deepest LOD level at once
int OctreeSendThread::packetDistributor(SharedNodePointer node, OctreeQueryNode* nodeData, bool viewFrustumChanged) {

    OctreeServer::didPacketDistributor(this);

    // if shutting down, exit early
    if (nodeData->isShuttingDown()) {
        return 0;
    }

    if (nodeData->elementBag.isEmpty()) {
        // if we're about to do a fresh pass,
        // give our pre-distribution processing a chance to do what it needs
        preDistributionProcessing();
    }

    _truePacketsSent = 0;
    _trueBytesSent = 0;
    _packetsSentThisInterval = 0;

    bool isFullScene = nodeData->shouldForceFullScene();
    if (isFullScene) {
        // we're forcing a full scene, clear the force in OctreeQueryNode so we don't force it next time again
        nodeData->setShouldForceFullScene(false);
    } else {
        // we aren't forcing a full scene, check if something else suggests we should
        isFullScene = nodeData->haveJSONParametersChanged() ||
            (nodeData->getUsesFrustum()
             && ((!viewFrustumChanged && nodeData->getViewFrustumJustStoppedChanging()) || nodeData->hasLodChanged()));
    }

    if (nodeData->isPacketWaiting()) {
        // send the waiting packet
        _packetsSentThisInterval += handlePacketSend(node, nodeData);
    } else {
        nodeData->resetOctreePacket();
    }
    int targetSize = MAX_OCTREE_PACKET_DATA_SIZE;
    targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);

    _packetData.changeSettings(true, targetSize); // FIXME - eventually support only compressed packets

    // If the current view frustum has changed OR we have nothing to send, then search against
    // the current view frustum for things to send.
    if (viewFrustumChanged || nodeData->elementBag.isEmpty()) {

        // if our view has changed, we need to reset these things...
        if (viewFrustumChanged) {
            if (nodeData->moveShouldDump() || nodeData->hasLodChanged()) {
                nodeData->dumpOutOfView();
            }
        }

        // track completed scenes and send out the stats packet accordingly
        nodeData->stats.sceneCompleted();
        nodeData->setLastRootTimestamp(_myServer->getOctree()->getRoot()->getLastChanged());
        _myServer->getOctree()->releaseSceneEncodeData(&nodeData->extraEncodeData);

        // TODO: add these to stats page
        //::endSceneSleepTime = _usleepTime;
        //unsigned long sleepTime = ::endSceneSleepTime - ::startSceneSleepTime;
        //unsigned long encodeTime = nodeData->stats.getTotalEncodeTime();
        //unsigned long elapsedTime = nodeData->stats.getElapsedTime();

        _packetsSentThisInterval += handlePacketSend(node, nodeData, isFullScene);

        // If we're starting a full scene, then definitely we want to empty the elementBag
        if (isFullScene) {
            nodeData->elementBag.deleteAll();
        }

        // TODO: add these to stats page
        //::startSceneSleepTime = _usleepTime;

        nodeData->sceneStart(usecTimestampNow() - CHANGE_FUDGE);
        // start tracking our stats
        nodeData->stats.sceneStarted(isFullScene, viewFrustumChanged,
                                     _myServer->getOctree()->getRoot(), _myServer->getJurisdiction());

        // This is the start of "resending" the scene.
        bool dontRestartSceneOnMove = false; // this is experimental
        if (dontRestartSceneOnMove) {
            if (nodeData->elementBag.isEmpty()) {
                nodeData->elementBag.insert(_myServer->getOctree()->getRoot());
            }
        } else {
            nodeData->elementBag.insert(_myServer->getOctree()->getRoot());
        }
    }

    // If we have something in our elementBag, then turn them into packets and send them out...
    if (!nodeData->elementBag.isEmpty()) {
        quint64 start = usecTimestampNow();

        traverseTreeAndSendContents(node, nodeData, viewFrustumChanged, isFullScene);

        // Here's where we can/should allow the server to send other data...
        // send the environment packet
        // TODO: should we turn this into a while loop to better handle sending multiple special packets
        if (_myServer->hasSpecialPacketsToSend(node) && !nodeData->isShuttingDown()) {
            int specialPacketsSent = 0;
            int specialBytesSent = _myServer->sendSpecialPackets(node, nodeData, specialPacketsSent);
            nodeData->resetOctreePacket();   // because nodeData's _sequenceNumber has changed
            _truePacketsSent += specialPacketsSent;
            _trueBytesSent += specialBytesSent;
            _packetsSentThisInterval += specialPacketsSent;

            _totalPackets += specialPacketsSent;
            _totalBytes += specialBytesSent;

            _totalSpecialPackets += specialPacketsSent;
            _totalSpecialBytes += specialBytesSent;
        }

        // calculate max number of packets that can be sent during this interval
        int clientMaxPacketsPerInterval = std::max(1, (nodeData->getMaxQueryPacketsPerSecond() / INTERVALS_PER_SECOND));
        int maxPacketsPerInterval = std::min(clientMaxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval());

        // Re-send packets that were nacked by the client
        while (nodeData->hasNextNackedPacket() && _packetsSentThisInterval < maxPacketsPerInterval) {
            const NLPacket* packet = nodeData->getNextNackedPacket();
            if (packet) {
                DependencyManager::get<NodeList>()->sendUnreliablePacket(*packet, *node);
                int numBytes = packet->getDataSize();
                _truePacketsSent++;
                _trueBytesSent += numBytes;
                _packetsSentThisInterval++;

                _totalPackets++;
                _totalBytes += numBytes;
                _totalWastedBytes += udt::MAX_PACKET_SIZE - packet->getDataSize();
            }
        }

        quint64 end = usecTimestampNow();
        int elapsedmsec = (end - start) / USECS_PER_MSEC;
        OctreeServer::trackLoopTime(elapsedmsec);

        // if after sending packets we've emptied our bag, then we want to remember that we've sent all
        // the octree elements from the current view frustum
        if (nodeData->elementBag.isEmpty()) {
            nodeData->updateLastKnownViewFrustum();
            nodeData->setViewSent(true);

            // If this was a full scene then make sure we really send out a stats packet at this point so that
            // the clients will know the scene is stable
            if (isFullScene) {
                nodeData->stats.sceneCompleted();
                handlePacketSend(node, nodeData, true);
            }
        }

    } // end if bag wasn't empty, and so we sent stuff...

    return _truePacketsSent;
}

void OctreeSendThread::traverseTreeAndSendContents(SharedNodePointer node, OctreeQueryNode* nodeData, bool viewFrustumChanged, bool isFullScene) {
    // calculate max number of packets that can be sent during this interval
    int clientMaxPacketsPerInterval = std::max(1, (nodeData->getMaxQueryPacketsPerSecond() / INTERVALS_PER_SECOND));
    int maxPacketsPerInterval = std::min(clientMaxPacketsPerInterval, _myServer->getPacketsPerClientPerInterval());

    int extraPackingAttempts = 0;
    bool completedScene = false;

    bool somethingToSend = true; // assume we have something
    while (somethingToSend && _packetsSentThisInterval < maxPacketsPerInterval && !nodeData->isShuttingDown()) {
        float lockWaitElapsedUsec = OctreeServer::SKIP_TIME;
        float encodeElapsedUsec = OctreeServer::SKIP_TIME;
        float compressAndWriteElapsedUsec = OctreeServer::SKIP_TIME;
        float packetSendingElapsedUsec = OctreeServer::SKIP_TIME;

        quint64 startInside = usecTimestampNow();

        bool lastNodeDidntFit = false; // assume each node fits
        if (!nodeData->elementBag.isEmpty()) {

            quint64 lockWaitStart = usecTimestampNow();
            _myServer->getOctree()->withReadLock([&]{
                quint64 lockWaitEnd = usecTimestampNow();
                lockWaitElapsedUsec = (float)(lockWaitEnd - lockWaitStart);
                quint64 encodeStart = usecTimestampNow();

                OctreeElementPointer subTree = nodeData->elementBag.extract();
                if (!subTree) {
                    return;
                }

                float octreeSizeScale = nodeData->getOctreeSizeScale();
                int boundaryLevelAdjustClient = nodeData->getBoundaryLevelAdjust();

                int boundaryLevelAdjust = boundaryLevelAdjustClient +
                                          (viewFrustumChanged ? LOW_RES_MOVING_ADJUST : NO_BOUNDARY_ADJUST);

                EncodeBitstreamParams params(INT_MAX, WANT_EXISTS_BITS, DONT_CHOP,
                                             viewFrustumChanged, boundaryLevelAdjust, octreeSizeScale,
                                             isFullScene, _myServer->getJurisdiction(), nodeData);
                nodeData->copyCurrentViewFrustum(params.viewFrustum);
                if (viewFrustumChanged) {
                    nodeData->copyLastKnownViewFrustum(params.lastViewFrustum);
                }

                // Our trackSend() function is implemented by the server subclass, and will be called back
                // during the encodeTreeBitstream() as new entities/data elements are sent
                params.trackSend = [this](const QUuid& dataID, quint64 dataEdited) {
                    _myServer->trackSend(dataID, dataEdited, _nodeUuid);
                };

                // TODO: should this include the lock time or not? This stat is sent down to the client,
                // it seems like it may be a good idea to include the lock time as part of the encode time
                // are reported to client. Since you can encode without the lock
                nodeData->stats.encodeStarted();

                // NOTE: this is where the tree "contents" are actaully packed
                _myServer->getOctree()->encodeTreeBitstream(subTree, &_packetData, nodeData->elementBag, params);

                quint64 encodeEnd = usecTimestampNow();
                encodeElapsedUsec = (float)(encodeEnd - encodeStart);

                // If after calling encodeTreeBitstream() there are no nodes left to send, then we know we've
                // sent the entire scene. We want to know this below so we'll actually write this content into
                // the packet and send it
                completedScene = nodeData->elementBag.isEmpty();

                if (params.stopReason == EncodeBitstreamParams::DIDNT_FIT) {
                    lastNodeDidntFit = true;
                    extraPackingAttempts++;
                }

                nodeData->stats.encodeStopped();
            });
        } else {
            somethingToSend = false; // this will cause us to drop out of the loop...
        }

        if (completedScene || lastNodeDidntFit) {
            // we probably want to flush what has accumulated in nodeData but:
            // do we have more data to send? and is there room?
            if (_packetData.hasContent()) {
                // yes, more data to send
                quint64 compressAndWriteStart = usecTimestampNow();
                unsigned int additionalSize = _packetData.getFinalizedSize() + sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
                if (additionalSize > nodeData->getAvailable()) {
                    // no room --> flush what we've got
                    _packetsSentThisInterval += handlePacketSend(node, nodeData);
                }

                // either there is room, or we've flushed and reset nodeData's data buffer
                // so we can transfer whatever is in _packetData to nodeData
                nodeData->writeToPacket(_packetData.getFinalizedData(), _packetData.getFinalizedSize());
                compressAndWriteElapsedUsec = (float)(usecTimestampNow()- compressAndWriteStart);
            }

            bool sendNow = completedScene ||
                nodeData->getAvailable() < MINIMUM_ATTEMPT_MORE_PACKING ||
                extraPackingAttempts > REASONABLE_NUMBER_OF_PACKING_ATTEMPTS;

            int targetSize = MAX_OCTREE_PACKET_DATA_SIZE;
            if (sendNow) {
                quint64 packetSendingStart = usecTimestampNow();
                _packetsSentThisInterval += handlePacketSend(node, nodeData);
                quint64 packetSendingEnd = usecTimestampNow();
                packetSendingElapsedUsec = (float)(packetSendingEnd - packetSendingStart);

                targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE);
                extraPackingAttempts = 0;
            } else {
                // We want to see if we have room for more in this wire packet but we've copied the _packetData,
                // so we want to start a new section. We will do that by resetting the packet settings with the max
                // size of our current available space in the wire packet plus room for our section header and a
                // little bit of padding.
                targetSize = nodeData->getAvailable() - sizeof(OCTREE_PACKET_INTERNAL_SECTION_SIZE) - COMPRESS_PADDING;
            }
            _packetData.changeSettings(true, targetSize); // will do reset - NOTE: Always compressed
        }
        OctreeServer::trackTreeWaitTime(lockWaitElapsedUsec);
        OctreeServer::trackEncodeTime(encodeElapsedUsec);
        OctreeServer::trackCompressAndWriteTime(compressAndWriteElapsedUsec);
        OctreeServer::trackPacketSendingTime(packetSendingElapsedUsec);

        quint64 endInside = usecTimestampNow();
        quint64 elapsedInsideUsecs = endInside - startInside;
        OctreeServer::trackInsideTime((float)elapsedInsideUsecs);
    }

    if (somethingToSend && _myServer->wantsVerboseDebug()) {
        qCDebug(octree) << "Hit PPS Limit, packetsSentThisInterval =" << _packetsSentThisInterval
                        << "  maxPacketsPerInterval = " << maxPacketsPerInterval
                        << "  clientMaxPacketsPerInterval = " << clientMaxPacketsPerInterval;
    }
}
