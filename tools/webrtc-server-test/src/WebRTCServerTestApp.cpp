//
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#define WEBRTC_POSIX 1
#define SIGSLOT_USE_POSIX_THREADS 1

#include <shared/WebRTC.h>

#include <api/jsep.h>
#include <api/peer_connection_interface.h>
#include <api/create_peerconnection_factory.h>
#include <api/jsep_ice_candidate.h>

#include <rtc_base/physical_socket_server.h>
#include <rtc_base/ssl_adapter.h>
#include <rtc_base/thread.h>
#include <rtc_base/logging.h>

#undef emit

#include <QDataStream>
#include <QTextStream>
#include <QThread>
#include <QFile>
#include <QLoggingCategory>
#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonObject>

#include <HTTPConnection.h>
#include <PathUtils.h>

#include <QtWebSockets/qwebsocket.h>
#include <QtWebSockets/qwebsocketserver.h>
#include <HTTPManager.h>

#include "WebRTCServerTestApp.h"


class WebRTCServer : public QObject, public HTTPRequestHandler {
    Q_OBJECT
public:
    WebRTCServer(bool verbose);
    ~WebRTCServer();

    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) override;
    void setupWebSocket();
    void onNewConnection();
    void processTextMessage(QString message);
    void processBinaryMessage(QByteArray message);
    void socketDisconnected();
    void closed();


    // WebRTC observer classes which bounce control back to WebRTCServer class

    class CreateSessionDescriptionObserver : public webrtc::CreateSessionDescriptionObserver {
    public:
        CreateSessionDescriptionObserver(WebRTCServer* owner) : _owner(owner) {}
        void OnSuccess(webrtc::SessionDescriptionInterface* desc) override { _owner->CreateSessionDescriptionObserver_OnSuccess(desc); }
        void OnFailure(webrtc::RTCError error) override { _owner->CreateSessionDescriptionObserver_OnFailure(error); };
        void OnFailure(const std::string& error) override { _owner->CreateSessionDescriptionObserver_OnFailure(error); }
        void AddRef() const override {}
        rtc::RefCountReleaseStatus Release() const override { return rtc::RefCountReleaseStatus(); }
    private:
        WebRTCServer* _owner { nullptr };
    };

    class SetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
    public:
        SetSessionDescriptionObserver(WebRTCServer* owner)  : _owner(owner) {}
        void OnSuccess() { _owner->SetSessionDescriptionObserver_OnSuccess(); }
        void OnFailure(const std::string& error) { _owner->SetSessionDescriptionObserver_OnFailure(error); }
        void AddRef() const {}
        rtc::RefCountReleaseStatus Release() const { return rtc::RefCountReleaseStatus(); }
    private:
        WebRTCServer* _owner { nullptr };
    };

    class PeerConnectionObserver : public webrtc::PeerConnectionObserver {
    public:
        PeerConnectionObserver(WebRTCServer* owner) : _owner(owner) {}
        virtual ~PeerConnectionObserver() {}
        void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
            _owner->PeerConnectionObserver_OnSignalingChange(new_state);
        }
        void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
            _owner->PeerConnectionObserver_OnAddStream(stream);
        }
        void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {
            _owner->PeerConnectionObserver_OnRemoveStream(stream);
        }
        void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {
            _owner->PeerConnectionObserver_OnDataChannel(channel);
        }
        void OnRenegotiationNeeded() override { _owner->PeerConnectionObserver_OnRenegotiationNeeded(); }
        void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
            _owner->PeerConnectionObserver_OnIceConnectionChange(new_state);
        }
        void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
            _owner->PeerConnectionObserver_OnIceGatheringChange(new_state);
        }
        void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {
            _owner->PeerConnectionObserver_OnIceCandidate(candidate);
        }
        void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) override {
            _owner->PeerConnectionObserver_OnConnectionChange(new_state);
        }
    private:
        WebRTCServer* _owner { nullptr };
    };

    class DataChannelObserver : public webrtc::DataChannelObserver {
    public:
        DataChannelObserver(WebRTCServer* owner)  : _owner(owner) {}
        void OnStateChange() override { _owner->DataChannelObserver_OnStateChange(); }
        void OnMessage(const webrtc::DataBuffer& buffer) override { _owner->DataChannelObserver_OnMessage(buffer); }
        void OnBufferedAmountChange(uint64_t previous_amount) override { _owner->DataChannelObserver_OnBufferedAmountChange(previous_amount); }
    private:
        WebRTCServer* _owner { nullptr };
    };


    void SetSessionDescriptionObserver_OnSuccess() {};
    void SetSessionDescriptionObserver_OnFailure(const std::string& error) {
        qDebug() << "=== SetSessionDescriptionObserver OnFailure" << error.c_str();
    };

    void CreateSessionDescriptionObserver_OnSuccess(webrtc::SessionDescriptionInterface* desc);

    void CreateSessionDescriptionObserver_OnFailure(const webrtc::RTCError& error) {
        qDebug() << "=== CreateSessionDescriptionObserver OnFailure A";
    }

    void CreateSessionDescriptionObserver_OnFailure(const std::string& error) {
        qDebug() << "=== CreateSessionDescriptionObserver OnFailure B" << error.c_str();
    }

    void PeerConnectionObserver_OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) {
        QString states[] = {
            "kStable",
            "kHaveLocalOffer",
            "kHaveLocalPrAnswer",
            "kHaveRemoteOffer",
            "kHaveRemotePrAnswer",
            "kClosed"
        };
        qDebug() << "=== PeerConnectionObserver OnSignalingChange" << states[(int)new_state];
    }

    void PeerConnectionObserver_OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
        qDebug() << "=== PeerConnectionObserver OnAddStream";
    }

    void PeerConnectionObserver_OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
        qDebug() << "=== PeerConnectionObserver OnRemoveStream";
    }

    void PeerConnectionObserver_OnRenegotiationNeeded() {
        qDebug() << "=== PeerConnectionObserver OnRenegotiationNeeded";
    }

    void PeerConnectionObserver_OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
        QString states[] = {
            "kIceConnectionNew",
            "kIceConnectionChecking",
            "kIceConnectionConnected",
            "kIceConnectionCompleted",
            "kIceConnectionFailed",
            "kIceConnectionDisconnected",
            "kIceConnectionClosed",
            "kIceConnectionMax"
        };
        qDebug() << "=== PeerConnectionObserver OnIceConnectionChange" << states[(int)new_state];
    }

    void PeerConnectionObserver_OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) {
        QString states[] = {
            "kIceGatheringNew",
            "kIceGatheringGathering",
            "kIceGatheringComplete"
        };
        qDebug() << "=== PeerConnectionObserver OnIceGatheringChange" << states[(int)new_state];
    }

    void PeerConnectionObserver_OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState new_state) {
        QString states[] = { "kNew", "kConnecting", "kConnected", "kDisconnected", "kFailed", "kClosed" };
        qDebug() << "=== PeerConnectionObserver OnConnectionChange" << states[(int)new_state];
    }

    void PeerConnectionObserver_OnIceCandidate(const webrtc::IceCandidateInterface* candidate);
    void PeerConnectionObserver_OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel);
    void DataChannelObserver_OnMessage(const webrtc::DataBuffer& buffer);

    void DataChannelObserver_OnStateChange() {
        qDebug() << "=== DataChannelObserver OnStateChange";
    }

    void DataChannelObserver_OnBufferedAmountChange(uint64_t previous_amount) {
        qDebug() << "=== DataChannelObserver OnBufferedAmountChange" << previous_amount;
    }


private:
    void processOffer(const QJsonDocument& messageJSON);

    bool _verbose { false };
    std::unique_ptr<HTTPManager> _httpManager;
    QWebSocketServer* _webSocketServer { nullptr };
    QWebSocket* _pSocket { nullptr };

    std::unique_ptr<rtc::Thread> _networkThread;
    std::unique_ptr<rtc::Thread> _workerThread;
    rtc::Thread* _signalingThread;
    rtc::scoped_refptr<webrtc::DataChannelInterface> _dataChannel;

    CreateSessionDescriptionObserver* create_session_description_observer { nullptr };
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection;
    SetSessionDescriptionObserver* set_session_description_observer { nullptr };
    PeerConnectionObserver* peer_connection_observer { nullptr };
    DataChannelObserver* data_channel_observer { nullptr };
};


WebRTCServer::WebRTCServer(bool verbose) :
    _verbose(verbose),
    _httpManager(nullptr) {

    rtc::LogMessage::LogToDebug(rtc::LERROR);
    rtc::LogMessage::LogThreads();
    rtc::LogMessage::LogTimestamps();

    int port = 8888;
    QString documentRoot = QString("%1/web").arg(PathUtils::getAppDataPath());
    _httpManager.reset(new HTTPManager(QHostAddress::AnyIPv4, port, documentRoot, this));

    rtc::InitializeSSL();
    rtc::InitRandom(rtc::Time());

    create_session_description_observer = new CreateSessionDescriptionObserver(this);
    set_session_description_observer = new SetSessionDescriptionObserver(this);
    peer_connection_observer = new PeerConnectionObserver(this);

    _networkThread = rtc::Thread::CreateWithSocketServer();
    _networkThread->Start();
    _workerThread = rtc::Thread::Create();
    _workerThread->Start();
    _signalingThread = rtc::Thread::Current();
    _signalingThread->Start();

    // set up websocket server from signaling thread (this one)
    setupWebSocket();

    webrtc::PeerConnectionFactoryDependencies pcf_deps;
    // pcf_deps.media_engine = cricket::CreateMediaEngine(std::move(media_deps));
    pcf_deps.fec_controller_factory = nullptr;
    pcf_deps.network_controller_factory = nullptr;
    pcf_deps.network_state_predictor_factory = nullptr;
    pcf_deps.media_transport_factory = nullptr;
    pcf_deps.network_thread = _networkThread.get();
    pcf_deps.worker_thread = _workerThread.get();
    pcf_deps.signaling_thread = _signalingThread;

    peer_connection_factory = CreateModularPeerConnectionFactory(std::move(pcf_deps));
}

WebRTCServer::~WebRTCServer() {
    delete create_session_description_observer;
}

void WebRTCServer::setupWebSocket() {
    _webSocketServer = new QWebSocketServer(QStringLiteral("Echo Server"), QWebSocketServer::NonSecureMode, this);
    if (_webSocketServer->listen(QHostAddress::Any, 8889)) {
        qDebug() << "WebSocketServer listen succeeded";
        connect(_webSocketServer, &QWebSocketServer::newConnection, this, &WebRTCServer::onNewConnection);
        connect(_webSocketServer, &QWebSocketServer::closed, this, &WebRTCServer::closed);
    } else {
        qDebug() << "WebSocketServer listen failed";
    }
}

bool WebRTCServer::handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) {
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation &&
        url.path() == "/something") {

        // QByteArray result =
        //     "<script>\n"
        //     "webSocket = new WebSocket(\"ws://chirp.house:8889/\");\n"
        //     "webSocket.onopen = function(e) {\n"
        //     "    webSocket.send(\"tes test test\");\n"
        //     "};\n"
        //     "</script>\n";


        QByteArray result =
            "<!DOCTYPE html>\n"
            "<meta charset=\"utf-8\"/>\n"
            "<html>\n"
            "  <head>\n"
            "    <script src=\"https://webrtc.github.io/adapter/adapter-latest.js\"></script>\n"
            "    <script src=\"http://headache.hungry.com/~seth/hifi/webrtc.js\"></script>\n"
            "  </head>\n"
            "\n"
            "  <body>\n"
            "    <video id=\"localVideo\" autoplay muted style=\"width:40%;\"></video>\n"
            "    <video id=\"remoteVideo\" autoplay style=\"width:40%;\"></video>\n"
            "\n"
            "    <br />\n"
            "\n"
            "    <input type=\"button\" id=\"start\" onclick=\"start(true)\" value=\"Start Video\"></input>\n"
            "\n"
            "    <script type=\"text/javascript\">\n"
            "        pageReady();\n"
            "    </script>\n"
            "  </body>\n"
            "</html>";
        connection->respond(HTTPConnection::StatusCode200, result, "text/html");
    } else {
        connection->respond(HTTPConnection::StatusCode404, HTTPConnection::StatusCode404);
    }
    return true;

}

void WebRTCServer::onNewConnection()
{
    _pSocket = _webSocketServer->nextPendingConnection();

    qDebug() << "new WebSocket connection";

    connect(_pSocket, &QWebSocket::textMessageReceived, this, &WebRTCServer::processTextMessage);
    connect(_pSocket, &QWebSocket::binaryMessageReceived, this, &WebRTCServer::processBinaryMessage);
    connect(_pSocket, &QWebSocket::disconnected, this, &WebRTCServer::socketDisconnected);
}


void WebRTCServer::processOffer(const QJsonDocument& messageJSON) {
    // QString type = messageJSON["sdp"]["type"].toString();
    QString sdp = messageJSON["sdp"]["sdp"].toString();

    webrtc::SdpParseError error;
    webrtc::SessionDescriptionInterface* session_description(
        webrtc::CreateSessionDescription("offer", sdp.toStdString(), &error));


    webrtc::PeerConnectionInterface::RTCConfiguration configuration;
    // webrtc::PeerConnectionInterface::IceServer ice_server;
    // ice_server.uri = "stun:stun.l.google.com:19302";
    // configuration.servers.push_back(ice_server);


    webrtc::PeerConnectionDependencies peerConnectionDependencies(peer_connection_observer);
    peer_connection = peer_connection_factory->CreatePeerConnection(configuration, std::move(peerConnectionDependencies));

    peer_connection->SetRemoteDescription(set_session_description_observer, session_description);

    // https://developer.mozilla.org/en-US/docs/Web/API/RTCOfferAnswerOptions
    webrtc::PeerConnectionInterface::RTCOfferAnswerOptions offerAnswerOptions(1, // int offer_to_receive_video,
                                                                              1, // int offer_to_receive_audio,
                                                                              1, // bool voice_activity_detection,
                                                                              false, // bool ice_restart,
                                                                              false // bool use_rtp_mux
        );
    // https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/createAnswer
    peer_connection->CreateAnswer(create_session_description_observer, offerAnswerOptions);
}


void WebRTCServer::processTextMessage(QString message) {

    qDebug() << "Message received:" << message;

    // {
    //   "sdp": {
    //     "type": "offer",
    //     "sdp": "v=0\r\no=mozilla...THIS_IS_SDPARTA-69.0.2 7249677672371419002 0 IN IP4 0.0.0.0\r\ns=-\r\nt=0 0\r\na=sendrecv\r\na=fingerprint:sha-256 7A:C3:44:A3:64:69:54:D1:E6:92:F1:7A:31:F5:47:23:A1:F1:E4:E2:19:09:8F:B8:91:6A:38:79:4D:8C:D2:0A\r\na=ice-options:trickle\r\na=msid-semantic:WMS *\r\n"
    //   },
    //   "uuid": "40970fec-d81f-a527-27ce-54f76ea1063b"
    // }

    QJsonParseError jsonError;
    QJsonDocument messageJSON = QJsonDocument::fromJson(message.toUtf8(), &jsonError);
    QJsonObject messageObject = messageJSON.object();
    if (jsonError.error == QJsonParseError::NoError) {
        if (messageObject.contains("sdp")) {
            processOffer(messageJSON);
        } else if (messageObject.contains("ice")) {

            // {
            //   "ice": {
            //     "candidate": "candidate:0 1 UDP 2122252543 10.246.56.237 43371 typ host",
            //     "sdpMid": "0",
            //     "sdpMLineIndex": 0,
            //     "usernameFragment": "066faa73"
            //   },
            //   "uuid": "36bc1307-4524-cc85-db6f-b61072cd7cdd"
            // }

            QJsonObject iceObject = messageObject["ice"].toObject();
            std::string candidate = iceObject["candidate"].toString().toUtf8().constData();
            int sdp_mline_index = iceObject["sdpMLineIndex"].toInt();
            std::string sdp_mid = iceObject["sdpMid"].toString().toUtf8().constData();

            if (candidate != "") {
                webrtc::SdpParseError error;
                auto candidate_object = webrtc::CreateIceCandidate(sdp_mid, sdp_mline_index, candidate, &error);
                peer_connection->AddIceCandidate(candidate_object);
            }
        } else {
            qDebug() << "no good";
        }
    } else {
        qDebug() << jsonError.errorString();
    }

}

void WebRTCServer::processBinaryMessage(QByteArray message) {
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    qDebug() << "Binary Message received:" << message;
    if (pClient) {
        pClient->sendBinaryMessage(message);
    }
}

void WebRTCServer::CreateSessionDescriptionObserver_OnSuccess(webrtc::SessionDescriptionInterface* desc) {
    std::string offer_string;
    desc->ToString(&offer_string);
    qDebug() << "=== OnAnswerCreated -- " << QString(offer_string.c_str());

    // https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/setLocalDescription
    peer_connection->SetLocalDescription(set_session_description_observer, desc);

    // {
    //     "type": "answer",
    //     "sdp": {
    //         "type": "answer",
    //         "sdp": ...,
    //     }
    // }

    QJsonObject responseJSONObject;
    responseJSONObject.insert("type", QJsonValue::fromVariant("answer"));
    QJsonObject payload;
    payload.insert("type", QJsonValue::fromVariant("answer"));
    payload.insert("sdp", QJsonValue::fromVariant(offer_string.c_str()));
    responseJSONObject.insert("sdp", payload);
    QJsonDocument responseJSONDocument(responseJSONObject);

    qDebug() << "sending to websocket: " << responseJSONDocument.toJson();
    /*qint64 result = */
    _pSocket->sendTextMessage(responseJSONDocument.toJson());
    // qDebug() << "send-result = " << result;

    // rtc::Thread::Current()->ProcessMessages(5 /* msec */);
}

void WebRTCServer::PeerConnectionObserver_OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
    qDebug() << "=== PeerConnectionObserver OnIceCandidate";

    std::string asString; // candidate:1837993626 1 udp 2122260223 10.246.56.237 59733 typ host generation 0 ufrag 5bTZ network-id 1 network-cost 50
    candidate->ToString(&asString);

    qDebug() << "PeerConnectionObserver OnIceCandidate again:" << asString.c_str();

    // {
    //   "ice": {
    //     "candidate": "candidate:0 1 UDP 2122252543 10.246.56.237 32806 typ host",
    //     "sdpMid": "0",
    //     "sdpMLineIndex": 0,
    //     "usernameFragment": "ea494d2a"
    //   },
    //   "uuid": "368eca6b-9991-a938-89fd-b00af49cfe6a"
    // }

    QJsonObject responseJSONObject;
    QJsonObject payload;
    payload.insert("candidate", QJsonValue::fromVariant(asString.c_str()));
    payload.insert("sdpMid", QJsonValue::fromVariant(candidate->sdp_mid().c_str()));
    payload.insert("sdpMLineIndex", QJsonValue::fromVariant(candidate->sdp_mline_index()));
    // payload.insert("usernameFragment", QJsonValue::fromVariant());
    responseJSONObject.insert("ice", payload);
    QJsonDocument responseJSONDocument(responseJSONObject);

    qDebug() << "sending to websocket: " << responseJSONDocument.toJson();
    /*qint64 result = */
    _pSocket->sendTextMessage(responseJSONDocument.toJson());
    // qDebug() << "send-result = " << result;
}

void WebRTCServer::PeerConnectionObserver_OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
    qDebug() << "=== PeerConnectionObserver OnDataChannel" << channel;
    data_channel_observer = new DataChannelObserver(this);
    channel->RegisterObserver(data_channel_observer);
    _dataChannel = channel;
}

void WebRTCServer::DataChannelObserver_OnMessage(const webrtc::DataBuffer& buffer) {
    std::string data(buffer.data.data<char>(), buffer.data.size());
    qDebug() << "=== DataChannelObserver OnMessage: " << data.c_str();

    std::string str = "pong";
    webrtc::DataBuffer resp(rtc::CopyOnWriteBuffer(str.c_str(), str.length()), false /* binary */);
    _dataChannel->Send(resp);
}


void WebRTCServer::socketDisconnected() {
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    qDebug() << "socketDisconnected:" << pClient;
    if (pClient) {
        pClient->deleteLater();
    }
}

void WebRTCServer::closed() {
    qDebug() << "closed";
}


WebRTCServerTestApp::WebRTCServerTestApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv) {

    QCommandLineParser parser;
    parser.setApplicationDescription("WebRTC Test Server");

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption verboseOutput("v", "verbose output");
    parser.addOption(verboseOutput);

    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText() << endl;
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        Q_UNREACHABLE();
    }

    bool verbose = parser.isSet(verboseOutput);


    WebRTCServer peer(verbose);

    while (true) {
        // oh I dont know.
        rtc::Thread::Current()->ProcessMessages(5 /* msec */);
        QCoreApplication::processEvents();
    }

    // #if USE_SSL
    //     net::SSLManager::destroy();
    // #endif
    //     rtc::CleanupSSL();
}

WebRTCServerTestApp::~WebRTCServerTestApp() {
}

#include "WebRTCServerTestApp.moc"
