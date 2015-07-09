//
//  Application.cpp
//  interface/src
//
//  Created by Andrzej Kapolka on 5/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <sstream>

#include <stdlib.h>
#include <cmath>
#include <math.h>

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/type_ptr.hpp>

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QAbstractNativeEventFilter>
#include <QActionGroup>
#include <QColorDialog>
#include <QCoreApplication>
#include <QDesktopWidget>
#include <QCheckBox>
#include <QImage>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMouseEvent>
#include <QNetworkReply>
#include <QNetworkDiskCache>
#include <QObject>
#include <QWheelEvent>
#include <QScreen>
#include <QShortcut>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QUrl>
#include <QWindow>
#include <QtDebug>
#include <QFileDialog>
#include <QDesktopServices>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QMediaPlayer>
#include <QMimeData>
#include <QMessageBox>
#include <QJsonDocument>

#include <AccountManager.h>
#include <AddressManager.h>
#include <CursorManager.h>
#include <AmbientOcclusionEffect.h>
#include <AudioInjector.h>
#include <AutoUpdater.h>
#include <DeferredLightingEffect.h>
#include <DependencyManager.h>
#include <EntityScriptingInterface.h>
#include <ErrorDialog.h>
#include <GlowEffect.h>
#include <gpu/Batch.h>
#include <gpu/Context.h>
#include <gpu/GLBackend.h>
#include <HFActionEvent.h>
#include <HFBackEvent.h>
#include <InfoView.h>
#include <LogHandler.h>
#include <MainWindow.h>
#include <MessageDialog.h>
#include <ModelEntityItem.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <ObjectMotionState.h>
#include <OctalCode.h>
#include <OctreeSceneStats.h>
#include <PacketHeaders.h>
#include <PathUtils.h>
#include <PerfStat.h>
#include <PhysicsEngine.h>
#include <ProgramObject.h>
#include <RenderDeferredTask.h>
#include <ResourceCache.h>
#include <SceneScriptingInterface.h>
#include <ScriptCache.h>
#include <SettingHandle.h>
#include <SimpleAverage.h>
#include <SoundCache.h>
#include <TextRenderer.h>
#include <Tooltip.h>
#include <UserActivityLogger.h>
#include <UUID.h>
#include <VrMenu.h>

#include "Application.h"
#include "AudioClient.h"
#include "DiscoverabilityManager.h"
#include "InterfaceVersion.h"
#include "LODManager.h"
#include "Menu.h"
#include "ModelPackager.h"
#include "Util.h"
#include "InterfaceLogging.h"
#include "InterfaceActionFactory.h"

#include "avatar/AvatarManager.h"

#include "audio/AudioIOStatsRenderer.h"
#include "audio/AudioScope.h"

#include "devices/DdeFaceTracker.h"
#include "devices/Faceshift.h"
#include "devices/Leapmotion.h"
#include "devices/RealSense.h"
#include "devices/SDL2Manager.h"
#include "devices/MIDIManager.h"
#include "devices/OculusManager.h"
#include "devices/TV3DManager.h"

#include "scripting/AccountScriptingInterface.h"
#include "scripting/AudioDeviceScriptingInterface.h"
#include "scripting/ClipboardScriptingInterface.h"
#include "scripting/HMDScriptingInterface.h"
#include "scripting/GlobalServicesScriptingInterface.h"
#include "scripting/LocationScriptingInterface.h"
#include "scripting/MenuScriptingInterface.h"
#include "scripting/SettingsScriptingInterface.h"
#include "scripting/WindowScriptingInterface.h"
#include "scripting/WebWindowClass.h"

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
#include "SpeechRecognizer.h"
#endif

#include "ui/AvatarInputs.h"
#include "ui/DataWebDialog.h"
#include "ui/DialogsManager.h"
#include "ui/LoginDialog.h"
#include "ui/Snapshot.h"
#include "ui/StandAloneJSConsole.h"
#include "ui/Stats.h"
#include "ui/AddressBarDialog.h"
#include "ui/UpdateDialog.h"


// ON WIndows PC, NVidia Optimus laptop, we want to enable NVIDIA GPU
#if defined(Q_OS_WIN)
extern "C" {
 _declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
}
#endif

enum CustomEventTypes {
    Lambda = QEvent::User + 1
};

class LambdaEvent : public QEvent {
    std::function<void()> _fun;
public:
    LambdaEvent(const std::function<void()> & fun) :
        QEvent(static_cast<QEvent::Type>(Lambda)), _fun(fun) {
    }
    LambdaEvent(std::function<void()> && fun) :
        QEvent(static_cast<QEvent::Type>(Lambda)), _fun(fun) {
    }
    void call() { _fun(); }
};


using namespace std;

//  Starfield information
static unsigned STARFIELD_NUM_STARS = 50000;
static unsigned STARFIELD_SEED = 1;
static uint8_t THROTTLED_IDLE_TIMER_DELAY = 10;

const qint64 MAXIMUM_CACHE_SIZE = 10 * BYTES_PER_GIGABYTES;  // 10GB

static QTimer* locationUpdateTimer = NULL;
static QTimer* balanceUpdateTimer = NULL;
static QTimer* identityPacketTimer = NULL;
static QTimer* billboardPacketTimer = NULL;
static QTimer* checkFPStimer = NULL;
static QTimer* idleTimer = NULL;

const QString CHECK_VERSION_URL = "https://highfidelity.com/latestVersion.xml";
const QString SKIP_FILENAME = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/hifi.skipversion";

const QString DEFAULT_SCRIPTS_JS_URL = "http://s3.amazonaws.com/hifi-public/scripts/defaultScripts.js";
Setting::Handle<int> maxOctreePacketsPerSecond("maxOctreePPS", DEFAULT_MAX_OCTREE_PPS);

#ifdef Q_OS_WIN
class MyNativeEventFilter : public QAbstractNativeEventFilter {
public:
    static MyNativeEventFilter& getInstance() {
        static MyNativeEventFilter staticInstance;
        return staticInstance;
    }

    bool nativeEventFilter(const QByteArray &eventType, void* msg, long* result) Q_DECL_OVERRIDE {
        if (eventType == "windows_generic_MSG") {
            MSG* message = (MSG*)msg;

            if (message->message == UWM_IDENTIFY_INSTANCES) {
                *result = UWM_IDENTIFY_INSTANCES;
                return true;
            }

            if (message->message == UWM_SHOW_APPLICATION) {
                MainWindow* applicationWindow = Application::getInstance()->getWindow();
                if (applicationWindow->isMinimized()) {
                    applicationWindow->showNormal();  // Restores to windowed or maximized state appropriately.
                }
                Application::getInstance()->setActiveWindow(applicationWindow);  // Flashes the taskbar icon if not focus.
                return true;
            }

            if (message->message == WM_COPYDATA) {
                COPYDATASTRUCT* pcds = (COPYDATASTRUCT*)(message->lParam);
                QUrl url = QUrl((const char*)(pcds->lpData));
                if (url.isValid() && url.scheme() == HIFI_URL_SCHEME) {
                    DependencyManager::get<AddressManager>()->handleLookupString(url.toString());
                    return true;
                }
            }
        }
        return false;
    }
};
#endif

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType) type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        Application::getInstance()->getLogger()->addMessage(qPrintable(logMessage + "\n"));
    }
}

bool setupEssentials(int& argc, char** argv) {
    unsigned int listenPort = 0; // bind to an ephemeral port by default
    const char** constArgv = const_cast<const char**>(argv);
    const char* portStr = getCmdOption(argc, constArgv, "--listenPort");
    if (portStr) {
        listenPort = atoi(portStr);
    }
    // Set build version
    QCoreApplication::setApplicationVersion(BUILD_VERSION);

    DependencyManager::registerInheritance<LimitedNodeList, NodeList>();
    DependencyManager::registerInheritance<AvatarHashMap, AvatarManager>();
    DependencyManager::registerInheritance<EntityActionFactoryInterface, InterfaceActionFactory>();

    Setting::init();

    // Set dependencies
    auto addressManager = DependencyManager::set<AddressManager>();
    auto nodeList = DependencyManager::set<NodeList>(NodeType::Agent, listenPort);
    auto geometryCache = DependencyManager::set<GeometryCache>();
    auto scriptCache = DependencyManager::set<ScriptCache>();
    auto soundCache = DependencyManager::set<SoundCache>();
    auto glowEffect = DependencyManager::set<GlowEffect>();
    auto faceshift = DependencyManager::set<Faceshift>();
    auto audio = DependencyManager::set<AudioClient>();
    auto audioScope = DependencyManager::set<AudioScope>();
    auto audioIOStatsRenderer = DependencyManager::set<AudioIOStatsRenderer>();
    auto deferredLightingEffect = DependencyManager::set<DeferredLightingEffect>();
    auto ambientOcclusionEffect = DependencyManager::set<AmbientOcclusionEffect>();
    auto textureCache = DependencyManager::set<TextureCache>();
    auto animationCache = DependencyManager::set<AnimationCache>();
    auto ddeFaceTracker = DependencyManager::set<DdeFaceTracker>();
    auto modelBlender = DependencyManager::set<ModelBlender>();
    auto avatarManager = DependencyManager::set<AvatarManager>();
    auto lodManager = DependencyManager::set<LODManager>();
    auto jsConsole = DependencyManager::set<StandAloneJSConsole>();
    auto dialogsManager = DependencyManager::set<DialogsManager>();
    auto bandwidthRecorder = DependencyManager::set<BandwidthRecorder>();
    auto resourceCacheSharedItems = DependencyManager::set<ResourceCacheSharedItems>();
    auto entityScriptingInterface = DependencyManager::set<EntityScriptingInterface>();
    auto windowScriptingInterface = DependencyManager::set<WindowScriptingInterface>();
#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    auto speechRecognizer = DependencyManager::set<SpeechRecognizer>();
#endif
    auto discoverabilityManager = DependencyManager::set<DiscoverabilityManager>();
    auto sceneScriptingInterface = DependencyManager::set<SceneScriptingInterface>();
    auto offscreenUi = DependencyManager::set<OffscreenUi>();
    auto autoUpdater = DependencyManager::set<AutoUpdater>();
    auto pathUtils = DependencyManager::set<PathUtils>();
    auto actionFactory = DependencyManager::set<InterfaceActionFactory>();

    return true;
}

Application::Application(int& argc, char** argv, QElapsedTimer &startup_time) :
        QApplication(argc, argv),
        _dependencyManagerIsSetup(setupEssentials(argc, argv)),
        _window(new MainWindow(desktop())),
        _toolWindow(NULL),
        _friendsWindow(NULL),
        _datagramProcessor(),
        _undoStack(),
        _undoStackScriptingInterface(&_undoStack),
        _frameCount(0),
        _fps(60.0f),
        _justStarted(true),
        _physicsEngine(glm::vec3(0.0f)),
        _entities(true, this, this),
        _entityClipboardRenderer(false, this, this),
        _entityClipboard(),
        _viewFrustum(),
        _lastQueriedViewFrustum(),
        _lastQueriedTime(usecTimestampNow()),
        _mirrorViewRect(QRect(MIRROR_VIEW_LEFT_PADDING, MIRROR_VIEW_TOP_PADDING, MIRROR_VIEW_WIDTH, MIRROR_VIEW_HEIGHT)),
        _firstRun("firstRun", true),
        _previousScriptLocation("LastScriptLocation"),
        _scriptsLocationHandle("scriptsLocation"),
        _fieldOfView("fieldOfView", DEFAULT_FIELD_OF_VIEW_DEGREES),
        _viewTransform(),
        _scaleMirror(1.0f),
        _rotateMirror(0.0f),
        _raiseMirror(0.0f),
        _cursorVisible(true),
        _lastMouseMove(usecTimestampNow()),
        _lastMouseMoveWasSimulated(false),
        _isTouchPressed(false),
        _mousePressed(false),
        _enableProcessOctreeThread(true),
        _octreeProcessor(),
        _nodeBoundsDisplay(this),
        _runningScriptsWidget(NULL),
        _runningScriptsWidgetWasVisible(false),
        _trayIcon(new QSystemTrayIcon(_window)),
        _lastNackTime(usecTimestampNow()),
        _lastSendDownstreamAudioStats(usecTimestampNow()),
        _isVSyncOn(true),
        _aboutToQuit(false),
        _notifiedPacketVersionMismatchThisDomain(false),
        _domainConnectionRefusals(QList<QString>()),
        _maxOctreePPS(maxOctreePacketsPerSecond.get()),
        _lastFaceTrackerUpdate(0),
        _applicationOverlay()
{
    setInstance(this);
#ifdef Q_OS_WIN
    installNativeEventFilter(&MyNativeEventFilter::getInstance());
#endif

    _logger = new FileLogger(this);  // After setting organization name in order to get correct directory

    qInstallMessageHandler(messageHandler);

    QFontDatabase::addApplicationFont(PathUtils::resourcesPath() + "styles/Inconsolata.otf");
    _window->setWindowTitle("Interface");

    Model::setAbstractViewStateInterface(this); // The model class will sometimes need to know view state details from us

    auto nodeList = DependencyManager::get<NodeList>();

    _myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

    _applicationStartupTime = startup_time;

    qCDebug(interfaceapp) << "[VERSION] Build sequence: " << qPrintable(applicationVersion());

    _bookmarks = new Bookmarks();  // Before setting up the menu

    _runningScriptsWidget = new RunningScriptsWidget(_window);
    _renderEngine->addTask(render::TaskPointer(new RenderDeferredTask()));
    _renderEngine->registerScene(_main3DScene);
      
    // start the nodeThread so its event loop is running
    QThread* nodeThread = new QThread(this);
    nodeThread->setObjectName("Datagram Processor Thread");
    nodeThread->start();

    // make sure the node thread is given highest priority
    nodeThread->setPriority(QThread::TimeCriticalPriority);

    _datagramProcessor = new DatagramProcessor(nodeList.data());

    // have the NodeList use deleteLater from DM customDeleter
    nodeList->setCustomDeleter([](Dependency* dependency) {
        static_cast<NodeList*>(dependency)->deleteLater();
    });

    // put the NodeList and datagram processing on the node thread
    nodeList->moveToThread(nodeThread);

    // geometry background downloads need to happen on the Datagram Processor Thread.  The idle loop will
    // emit checkBackgroundDownloads to cause the GeometryCache to check it's queue for requested background
    // downloads.
    QSharedPointer<GeometryCache> geometryCacheP = DependencyManager::get<GeometryCache>();
    ResourceCache* geometryCache = geometryCacheP.data();
    connect(this, &Application::checkBackgroundDownloads, geometryCache, &ResourceCache::checkAsynchronousGets);

    // connect the DataProcessor processDatagrams slot to the QUDPSocket readyRead() signal
    connect(&nodeList->getNodeSocket(), &QUdpSocket::readyRead, _datagramProcessor, &DatagramProcessor::processDatagrams);

    // put the audio processing on a separate thread
    QThread* audioThread = new QThread();
    audioThread->setObjectName("Audio Thread");

    auto audioIO = DependencyManager::get<AudioClient>();

    audioIO->setPositionGetter(getPositionForAudio);
    audioIO->setOrientationGetter(getOrientationForAudio);

    audioIO->moveToThread(audioThread);
    connect(audioThread, &QThread::started, audioIO.data(), &AudioClient::start);
    connect(audioIO.data(), &AudioClient::destroyed, audioThread, &QThread::quit);
    connect(audioThread, &QThread::finished, audioThread, &QThread::deleteLater);
    connect(audioIO.data(), &AudioClient::muteToggled, this, &Application::audioMuteToggled);
    connect(audioIO.data(), &AudioClient::receivedFirstPacket,
            &AudioScriptingInterface::getInstance(), &AudioScriptingInterface::receivedFirstPacket);
    connect(audioIO.data(), &AudioClient::disconnected,
            &AudioScriptingInterface::getInstance(), &AudioScriptingInterface::disconnected);

    audioThread->start();

    const DomainHandler& domainHandler = nodeList->getDomainHandler();

    connect(&domainHandler, SIGNAL(hostnameChanged(const QString&)), SLOT(domainChanged(const QString&)));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(connectedToDomain(const QString&)));
    connect(&domainHandler, SIGNAL(connectedToDomain(const QString&)), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(updateWindowTitle()));
    connect(&domainHandler, SIGNAL(disconnectedFromDomain()), SLOT(clearDomainOctreeDetails()));
    connect(&domainHandler, &DomainHandler::settingsReceived, this, &Application::domainSettingsReceived);
    connect(&domainHandler, &DomainHandler::hostnameChanged,
            DependencyManager::get<AddressManager>().data(), &AddressManager::storeCurrentAddress);

    // update our location every 5 seconds in the metaverse server, assuming that we are authenticated with one
    const qint64 DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS = 5 * 1000;

    locationUpdateTimer = new QTimer(this);
    auto discoverabilityManager = DependencyManager::get<DiscoverabilityManager>();
    connect(locationUpdateTimer, &QTimer::timeout, discoverabilityManager.data(), &DiscoverabilityManager::updateLocation);
    locationUpdateTimer->start(DATA_SERVER_LOCATION_CHANGE_UPDATE_MSECS);

    // if we get a domain change, immediately attempt update location in metaverse server
    connect(&nodeList->getDomainHandler(), &DomainHandler::connectedToDomain,
            discoverabilityManager.data(), &DiscoverabilityManager::updateLocation);

    connect(nodeList.data(), &NodeList::nodeAdded, this, &Application::nodeAdded);
    connect(nodeList.data(), &NodeList::nodeKilled, this, &Application::nodeKilled);
    connect(nodeList.data(), SIGNAL(nodeKilled(SharedNodePointer)), SLOT(nodeKilled(SharedNodePointer)));
    connect(nodeList.data(), &NodeList::uuidChanged, _myAvatar, &MyAvatar::setSessionUUID);
    connect(nodeList.data(), &NodeList::uuidChanged, this, &Application::setSessionUUID);
    connect(nodeList.data(), &NodeList::limitOfSilentDomainCheckInsReached, nodeList.data(), &NodeList::reset);
    connect(nodeList.data(), &NodeList::packetVersionMismatch, this, &Application::notifyPacketVersionMismatch);

    // connect to appropriate slots on AccountManager
    AccountManager& accountManager = AccountManager::getInstance();

    const qint64 BALANCE_UPDATE_INTERVAL_MSECS = 5 * 1000;

    balanceUpdateTimer = new QTimer(this);
    connect(balanceUpdateTimer, &QTimer::timeout, &accountManager, &AccountManager::updateBalance);
    balanceUpdateTimer->start(BALANCE_UPDATE_INTERVAL_MSECS);

    connect(&accountManager, &AccountManager::balanceChanged, this, &Application::updateWindowTitle);

    auto dialogsManager = DependencyManager::get<DialogsManager>();
    connect(&accountManager, &AccountManager::authRequired, dialogsManager.data(), &DialogsManager::showLoginDialog);
    connect(&accountManager, &AccountManager::usernameChanged, this, &Application::updateWindowTitle);

    // set the account manager's root URL and trigger a login request if we don't have the access token
    accountManager.setAuthURL(NetworkingConstants::METAVERSE_SERVER_URL);
    UserActivityLogger::getInstance().launch(applicationVersion());

    // once the event loop has started, check and signal for an access token
    QMetaObject::invokeMethod(&accountManager, "checkAndSignalForAccessToken", Qt::QueuedConnection);

    auto addressManager = DependencyManager::get<AddressManager>();

    // use our MyAvatar position and quat for address manager path
    addressManager->setPositionGetter(getPositionForPath);
    addressManager->setOrientationGetter(getOrientationForPath);

    connect(addressManager.data(), &AddressManager::hostChanged, this, &Application::updateWindowTitle);
    connect(this, &QCoreApplication::aboutToQuit, addressManager.data(), &AddressManager::storeCurrentAddress);

    #ifdef _WIN32
    WSADATA WsaData;
    int wsaresult = WSAStartup(MAKEWORD(2,2), &WsaData);
    #endif

    // tell the NodeList instance who to tell the domain server we care about
    nodeList->addSetOfNodeTypesToNodeInterestSet(NodeSet() << NodeType::AudioMixer << NodeType::AvatarMixer
                                                 << NodeType::EntityServer);

    // connect to the packet sent signal of the _entityEditSender
    connect(&_entityEditSender, &EntityEditPacketSender::packetSent, this, &Application::packetSent);

    // send the identity packet for our avatar each second to our avatar mixer
    identityPacketTimer = new QTimer();
    connect(identityPacketTimer, &QTimer::timeout, _myAvatar, &MyAvatar::sendIdentityPacket);
    identityPacketTimer->start(AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS);

    // send the billboard packet for our avatar every few seconds
    billboardPacketTimer = new QTimer();
    connect(billboardPacketTimer, &QTimer::timeout, _myAvatar, &MyAvatar::sendBillboardPacket);
    billboardPacketTimer->start(AVATAR_BILLBOARD_PACKET_SEND_INTERVAL_MSECS);

    QString cachePath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkDiskCache* cache = new QNetworkDiskCache();
    cache->setMaximumCacheSize(MAXIMUM_CACHE_SIZE);
    cache->setCacheDirectory(!cachePath.isEmpty() ? cachePath : "interfaceCache");
    networkAccessManager.setCache(cache);

    ResourceCache::setRequestLimit(3);

    _window->setCentralWidget(_glWidget);

    _window->restoreGeometry();

    _window->setVisible(true);
    _glWidget->setFocusPolicy(Qt::StrongFocus);
    _glWidget->setFocus();
#ifdef Q_OS_MAC
    // OSX doesn't seem to provide for hiding the cursor only on the GL widget
    _window->setCursor(Qt::BlankCursor);
#else
    // On windows and linux, hiding the top level cursor also means it's invisible
    // when hovering over the window menu, which is a pain, so only hide it for
    // the GL surface
    _glWidget->setCursor(Qt::BlankCursor);
#endif
    
    // enable mouse tracking; otherwise, we only get drag events
    _glWidget->setMouseTracking(true);

    _fullscreenMenuWidget->setParent(_glWidget);
    _menuBarHeight = Menu::getInstance()->height();
    if (Menu::getInstance()->isOptionChecked(MenuOption::Fullscreen)) {
        setFullscreen(true);  // Initialize menu bar show/hide
    }

    _toolWindow = new ToolWindow();
    _toolWindow->setWindowFlags(_toolWindow->windowFlags() | Qt::WindowStaysOnTopHint);
    _toolWindow->setWindowTitle("Tools");

    // initialization continues in initializeGL when OpenGL context is ready

    // Tell our entity edit sender about our known jurisdictions
    _entityEditSender.setServerJurisdictions(&_entityServerJurisdictions);

    // For now we're going to set the PPS for outbound packets to be super high, this is
    // probably not the right long term solution. But for now, we're going to do this to
    // allow you to move an entity around in your hand
    _entityEditSender.setPacketsPerSecond(3000); // super high!!

    _overlays.init(); // do this before scripts load

    _runningScriptsWidget->setRunningScripts(getRunningScripts());
    connect(_runningScriptsWidget, &RunningScriptsWidget::stopScriptName, this, &Application::stopScript);

    connect(this, SIGNAL(aboutToQuit()), this, SLOT(saveScripts()));
    connect(this, SIGNAL(aboutToQuit()), this, SLOT(aboutToQuit()));

    // hook up bandwidth estimator
    QSharedPointer<BandwidthRecorder> bandwidthRecorder = DependencyManager::get<BandwidthRecorder>();
    connect(nodeList.data(), SIGNAL(dataSent(const quint8, const int)),
            bandwidthRecorder.data(), SLOT(updateOutboundData(const quint8, const int)));
    connect(nodeList.data(), SIGNAL(dataReceived(const quint8, const int)),
            bandwidthRecorder.data(), SLOT(updateInboundData(const quint8, const int)));

    connect(&_myAvatar->getSkeletonModel(), &SkeletonModel::skeletonLoaded,
            this, &Application::checkSkeleton, Qt::QueuedConnection);

    // Setup the userInputMapper with the actions
    // Setup the keyboardMouseDevice and the user input mapper with the default bindings
    _keyboardMouseDevice.registerToUserInputMapper(_userInputMapper);
    _keyboardMouseDevice.assignDefaultInputMapping(_userInputMapper);

    // check first run...
    if (_firstRun.get()) {
        qCDebug(interfaceapp) << "This is a first run...";
        // clear the scripts, and set out script to our default scripts
        clearScriptsBeforeRunning();
        loadScript(DEFAULT_SCRIPTS_JS_URL);

        _firstRun.set(false);
    } else {
        // do this as late as possible so that all required subsystems are initialized
        loadScripts();
    }

    loadSettings();
    int SAVE_SETTINGS_INTERVAL = 10 * MSECS_PER_SECOND; // Let's save every seconds for now
    connect(&_settingsTimer, &QTimer::timeout, this, &Application::saveSettings);
    connect(&_settingsThread, SIGNAL(started()), &_settingsTimer, SLOT(start()));
    connect(&_settingsThread, SIGNAL(finished()), &_settingsTimer, SLOT(stop()));
    _settingsTimer.moveToThread(&_settingsThread);
    _settingsTimer.setSingleShot(false);
    _settingsTimer.setInterval(SAVE_SETTINGS_INTERVAL);
    _settingsThread.start();
    
    if (Menu::getInstance()->isOptionChecked(MenuOption::IndependentMode)) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
        cameraMenuChanged();
    }

    _trayIcon->show();

    // set the local loopback interface for local sounds from audio scripts
    AudioScriptingInterface::getInstance().setLocalAudioInterface(audioIO.data());

#ifdef HAVE_RTMIDI
    // setup the MIDIManager
    MIDIManager& midiManagerInstance = MIDIManager::getInstance();
    midiManagerInstance.openDefaultPort();
#endif

    this->installEventFilter(this);
    // The offscreen UI needs to intercept the mouse and keyboard
    // events coming from the onscreen window
    _glWidget->installEventFilter(DependencyManager::get<OffscreenUi>().data());

    // initialize our face trackers after loading the menu settings
    auto faceshiftTracker = DependencyManager::get<Faceshift>();
    faceshiftTracker->init();
    connect(faceshiftTracker.data(), &FaceTracker::muteToggled, this, &Application::faceTrackerMuteToggled);
#ifdef HAVE_DDE
    auto ddeTracker = DependencyManager::get<DdeFaceTracker>();
    ddeTracker->init();
    connect(ddeTracker.data(), &FaceTracker::muteToggled, this, &Application::faceTrackerMuteToggled);
#endif
    
    auto applicationUpdater = DependencyManager::get<AutoUpdater>();
    connect(applicationUpdater.data(), &AutoUpdater::newVersionIsAvailable, dialogsManager.data(), &DialogsManager::showUpdateDialog);
    applicationUpdater->checkForUpdate();
}

void Application::aboutToQuit() {
    emit beforeAboutToQuit();

    _aboutToQuit = true;
    cleanupBeforeQuit();
}

void Application::cleanupBeforeQuit() {

    _entities.clear(); // this will allow entity scripts to properly shutdown

    _datagramProcessor->shutdown(); // tell the datagram processor we're shutting down, so it can short circuit
    _entities.shutdown(); // tell the entities system we're shutting down, so it will stop running scripts
    ScriptEngine::stopAllScripts(this); // stop all currently running global scripts

    // first stop all timers directly or by invokeMethod
    // depending on what thread they run in
    locationUpdateTimer->stop();
    balanceUpdateTimer->stop();
    identityPacketTimer->stop();
    billboardPacketTimer->stop();
    checkFPStimer->stop();
    idleTimer->stop();
    QMetaObject::invokeMethod(&_settingsTimer, "stop", Qt::BlockingQueuedConnection);

    // and then delete those that got created by "new"
    delete locationUpdateTimer;
    delete balanceUpdateTimer;
    delete identityPacketTimer;
    delete billboardPacketTimer;
    delete checkFPStimer;
    delete idleTimer;
    // no need to delete _settingsTimer here as it is no pointer

    // save state
    _settingsThread.quit();
    saveSettings();
    _window->saveGeometry();

    // let the avatar mixer know we're out
    MyAvatar::sendKillAvatar();

    // stop the AudioClient
    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(),
                              "stop", Qt::BlockingQueuedConnection);

    // destroy the AudioClient so it and its thread have a chance to go down safely
    DependencyManager::destroy<AudioClient>();

#ifdef HAVE_DDE
    DependencyManager::destroy<DdeFaceTracker>();
#endif
}

Application::~Application() {
    EntityTree* tree = _entities.getTree();
    tree->lockForWrite();
    _entities.getTree()->setSimulation(NULL);
    tree->unlock();

    _octreeProcessor.terminate();
    _entityEditSender.terminate();

    Menu::getInstance()->deleteLater();

    _physicsEngine.setCharacterController(NULL);
    _myAvatar = NULL;

    ModelEntityItem::cleanupLoadedAnimations();

    // stop the glWidget frame timer so it doesn't call paintGL
    _glWidget->stopFrameTimer();

    // remove avatars from physics engine
    DependencyManager::get<AvatarManager>()->clearOtherAvatars();
    _physicsEngine.deleteObjects(DependencyManager::get<AvatarManager>()->getObjectsToDelete());

    DependencyManager::destroy<OffscreenUi>();
    DependencyManager::destroy<AvatarManager>();
    DependencyManager::destroy<AnimationCache>();
    DependencyManager::destroy<TextureCache>();
    DependencyManager::destroy<GeometryCache>();
    DependencyManager::destroy<ScriptCache>();
    DependencyManager::destroy<SoundCache>();

    QThread* nodeThread = DependencyManager::get<NodeList>()->thread();
    DependencyManager::destroy<NodeList>();

    // ask the node thread to quit and wait until it is done
    nodeThread->quit();
    nodeThread->wait();

    Leapmotion::destroy();
    RealSense::destroy();

    qInstallMessageHandler(NULL); // NOTE: Do this as late as possible so we continue to get our log messages
}

void Application::initializeGL() {
    qCDebug(interfaceapp) << "Created Display Window.";

    // initialize glut for shape drawing; Qt apparently initializes it on OS X
    #ifndef __APPLE__
    static bool isInitialized = false;
    if (isInitialized) {
        return;
    } else {
        isInitialized = true;
    }
    #endif

    qCDebug(interfaceapp) << "GL Version: " << QString((const char*) glGetString(GL_VERSION));
    qCDebug(interfaceapp) << "GL Shader Language Version: " << QString((const char*) glGetString(GL_SHADING_LANGUAGE_VERSION));
    qCDebug(interfaceapp) << "GL Vendor: " << QString((const char*) glGetString(GL_VENDOR));
    qCDebug(interfaceapp) << "GL Renderer: " << QString((const char*) glGetString(GL_RENDERER));

    #ifdef WIN32
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        qCDebug(interfaceapp, "Error: %s\n", glewGetErrorString(err));
    }
    qCDebug(interfaceapp, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

    if (wglewGetExtension("WGL_EXT_swap_control")) {
        int swapInterval = wglGetSwapIntervalEXT();
        qCDebug(interfaceapp, "V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    }
    #endif

#if defined(Q_OS_LINUX)
    // TODO: Write the correct  code for Linux...
    /* if (wglewGetExtension("WGL_EXT_swap_control")) {
        int swapInterval = wglGetSwapIntervalEXT();
        qCDebug(interfaceapp, "V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    }*/
#endif

    initDisplay();
    qCDebug(interfaceapp, "Initialized Display.");

    // The UI can't be created until the primary OpenGL
    // context is created, because it needs to share
    // texture resources
    initializeUi();
    qCDebug(interfaceapp, "Initialized Offscreen UI.");
    _glWidget->makeCurrent();

    // call Menu getInstance static method to set up the menu
    // Needs to happen AFTER the QML UI initialization
    _window->setMenuBar(Menu::getInstance());

    init();
    qCDebug(interfaceapp, "init() complete.");

    // create thread for parsing of octee data independent of the main network and rendering threads
    _octreeProcessor.initialize(_enableProcessOctreeThread);
    connect(&_octreeProcessor, &OctreePacketProcessor::packetVersionMismatch, this, &Application::notifyPacketVersionMismatch);
    _entityEditSender.initialize(_enableProcessOctreeThread);

    // call our timer function every second
    checkFPStimer = new QTimer(this);
    connect(checkFPStimer, SIGNAL(timeout()), SLOT(checkFPS()));
    checkFPStimer->start(1000);

    // call our idle function whenever we can
    idleTimer = new QTimer(this);
    connect(idleTimer, SIGNAL(timeout()), SLOT(idle()));
    idleTimer->start(0);
    _idleLoopStdev.reset();

    if (_justStarted) {
        float startupTime = (float)_applicationStartupTime.elapsed() / 1000.0f;
        _justStarted = false;
        qCDebug(interfaceapp, "Startup time: %4.2f seconds.", (double)startupTime);
    }

    // update before the first render
    update(1.0f / _fps);

    InfoView::show(INFO_HELP_PATH, true);
}

void Application::initializeUi() {
    AddressBarDialog::registerType();
    ErrorDialog::registerType();
    LoginDialog::registerType();
    MessageDialog::registerType();
    VrMenu::registerType();
    Tooltip::registerType();
    UpdateDialog::registerType();

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->create(_glWidget->context()->contextHandle());
    offscreenUi->resize(_glWidget->size());
    offscreenUi->setProxyWindow(_window->windowHandle());
    offscreenUi->setBaseUrl(QUrl::fromLocalFile(PathUtils::resourcesPath() + "/qml/"));
    offscreenUi->load("Root.qml");
    offscreenUi->load("RootMenu.qml");
    VrMenu::load();
    VrMenu::executeQueuedLambdas();
    offscreenUi->setMouseTranslator([this](const QPointF& p){
        if (OculusManager::isConnected()) {
            glm::vec2 pos = _compositor.screenToOverlay(toGlm(p));
            return QPointF(pos.x, pos.y);
        }
        return QPointF(p);
    });
    offscreenUi->resume();
    connect(_window, &MainWindow::windowGeometryChanged, [this](const QRect & r){
        static qreal oldDevicePixelRatio = 0;
        qreal devicePixelRatio = _glWidget->devicePixelRatio();
        if (devicePixelRatio != oldDevicePixelRatio) {
            oldDevicePixelRatio = devicePixelRatio;
            qDebug() << "Device pixel ratio changed, triggering GL resize";
            resizeGL();
        }
    });
}

void Application::paintGL() {
    PROFILE_RANGE(__FUNCTION__);
    _glWidget->makeCurrent();

    auto lodManager = DependencyManager::get<LODManager>();
    gpu::Context context(new gpu::GLBackend());
    RenderArgs renderArgs(&context, nullptr, getViewFrustum(), lodManager->getOctreeSizeScale(),
                          lodManager->getBoundaryLevelAdjust(), RenderArgs::DEFAULT_RENDER_MODE,
                          RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);

    PerformanceTimer perfTimer("paintGL");
    //Need accurate frame timing for the oculus rift
    if (OculusManager::isConnected()) {
        OculusManager::beginFrameTiming();
    }

  
    PerformanceWarning::setSuppressShortTimings(Menu::getInstance()->isOptionChecked(MenuOption::SuppressShortTimings));
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::paintGL()");
    resizeGL();

    {
        PerformanceTimer perfTimer("renderOverlay");
        /*
        gpu::Context context(new gpu::GLBackend());
        RenderArgs renderArgs(&context, nullptr, getViewFrustum(), lodManager->getOctreeSizeScale(),
            lodManager->getBoundaryLevelAdjust(), RenderArgs::DEFAULT_RENDER_MODE,
            RenderArgs::MONO, RenderArgs::RENDER_DEBUG_NONE);
            */
        _applicationOverlay.renderOverlay(&renderArgs);
    }

    glEnable(GL_LINE_SMOOTH);
    
    if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON || _myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
        Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, _myAvatar->getBoomLength() <= MyAvatar::ZOOM_MIN);
        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, !(_myAvatar->getBoomLength() <= MyAvatar::ZOOM_MIN));
        Application::getInstance()->cameraMenuChanged();
    }

    if (_myCamera.getMode() == CAMERA_MODE_FIRST_PERSON) {
        // Always use the default eye position, not the actual head eye position.
        // Using the latter will cause the camera to wobble with idle animations,
        // or with changes from the face tracker
        _myCamera.setPosition(_myAvatar->getDefaultEyePosition());
        if (!OculusManager::isConnected()) {
            // If not using an HMD, grab the camera orientation directly
            _myCamera.setRotation(_myAvatar->getHead()->getCameraOrientation());
        } else {
            // In an HMD, people can look up and down with their actual neck, and the
            // per-eye HMD pose will be applied later.  So set the camera orientation
            // to only the yaw, excluding pitch and roll, i.e. an orientation that
            // is orthongonal to the (body's) Y axis
            _myCamera.setRotation(_myAvatar->getWorldAlignedOrientation());
        }

    } else if (_myCamera.getMode() == CAMERA_MODE_THIRD_PERSON) {
        if (OculusManager::isConnected()) {
            _myCamera.setRotation(_myAvatar->getWorldAlignedOrientation());
        } else {
            _myCamera.setRotation(_myAvatar->getHead()->getOrientation());
        }
        if (Menu::getInstance()->isOptionChecked(MenuOption::CenterPlayerInView)) {
            _myCamera.setPosition(_myAvatar->getDefaultEyePosition() +
                                  _myCamera.getRotation() * glm::vec3(0.0f, 0.0f, 1.0f) * _myAvatar->getBoomLength() * _myAvatar->getScale());
        } else {
            _myCamera.setPosition(_myAvatar->getDefaultEyePosition() +
                                  _myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, 1.0f) * _myAvatar->getBoomLength() * _myAvatar->getScale());
        }

    } else if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        _myCamera.setRotation(_myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI + _rotateMirror, 0.0f)));
        _myCamera.setPosition(_myAvatar->getDefaultEyePosition() +
                              glm::vec3(0, _raiseMirror * _myAvatar->getScale(), 0) +
                              (_myAvatar->getOrientation() * glm::quat(glm::vec3(0.0f, _rotateMirror, 0.0f))) *
                               glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_FULLSCREEN_DISTANCE * _scaleMirror);
    }

    // Update camera position
    if (!OculusManager::isConnected()) {
        _myCamera.update(1.0f / _fps);
    }

    // Sync up the View Furstum with the camera
    // FIXME: it's happening again in the updateSHadow and it shouldn't, this should be the place
    loadViewFrustum(_myCamera, _viewFrustum);

    if (getShadowsEnabled()) {
        renderArgs._renderMode = RenderArgs::SHADOW_RENDER_MODE;
        updateShadowMap(&renderArgs);
    }

    renderArgs._renderMode = RenderArgs::DEFAULT_RENDER_MODE;

    if (OculusManager::isConnected()) {
        //When in mirror mode, use camera rotation. Otherwise, use body rotation
        if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
            OculusManager::display(_glWidget, &renderArgs, _myCamera.getRotation(), _myCamera.getPosition(), _myCamera);
        } else {
            OculusManager::display(_glWidget, &renderArgs, _myAvatar->getWorldAlignedOrientation(), _myAvatar->getDefaultEyePosition(), _myCamera);
        }
    } else if (TV3DManager::isConnected()) {
        TV3DManager::display(&renderArgs, _myCamera);
    } else {
        DependencyManager::get<GlowEffect>()->prepare(&renderArgs);

        // Viewport is assigned to the size of the framebuffer
        QSize size = DependencyManager::get<TextureCache>()->getFrameBufferSize();
        glViewport(0, 0, size.width(), size.height());

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        displaySide(&renderArgs, _myCamera);
        glPopMatrix();

        renderArgs._renderMode = RenderArgs::MIRROR_RENDER_MODE;
        if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
            renderRearViewMirror(&renderArgs, _mirrorViewRect);       
        }

        renderArgs._renderMode = RenderArgs::NORMAL_RENDER_MODE;

        auto finalFbo = DependencyManager::get<GlowEffect>()->render(&renderArgs);


        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(finalFbo));
        glBlitFramebuffer(0, 0, _renderResolution.x, _renderResolution.y,
                          0, 0, _glWidget->getDeviceSize().width(), _glWidget->getDeviceSize().height(),
                            GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

        _compositor.displayOverlayTexture(&renderArgs);
    }


    if (!OculusManager::isConnected() || OculusManager::allowSwap()) {
        _glWidget->swapBuffers();
    }

    if (OculusManager::isConnected()) {
        OculusManager::endFrameTiming();
    }
    _frameCount++;
    Stats::getInstance()->setRenderDetails(renderArgs._details);
}

void Application::runTests() {
    runTimingTests();
}

void Application::audioMuteToggled() {
    QAction* muteAction = Menu::getInstance()->getActionForOption(MenuOption::MuteAudio);
    Q_CHECK_PTR(muteAction);
    muteAction->setChecked(DependencyManager::get<AudioClient>()->isMuted());
}

void Application::faceTrackerMuteToggled() {
    QAction* muteAction = Menu::getInstance()->getActionForOption(MenuOption::MuteFaceTracking);
    Q_CHECK_PTR(muteAction);
    bool isMuted = getSelectedFaceTracker()->isMuted();
    muteAction->setChecked(isMuted);
    getSelectedFaceTracker()->setEnabled(!isMuted);
    Menu::getInstance()->getActionForOption(MenuOption::CalibrateCamera)->setEnabled(!isMuted);
}

void Application::aboutApp() {
    InfoView::show(INFO_HELP_PATH);
}

void Application::showEditEntitiesHelp() {
    InfoView::show(INFO_EDIT_ENTITIES_PATH);
}

void Application::resetCameras(Camera& camera, const glm::uvec2& size) {
    if (OculusManager::isConnected()) {
        OculusManager::configureCamera(camera);
    } else if (TV3DManager::isConnected()) {
        TV3DManager::configureCamera(camera, size.x, size.y);
    } else {
        camera.setProjection(glm::perspective(glm::radians(_fieldOfView.get()), (float)size.x / size.y, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
    }
}

void Application::resizeGL() {
    // Set the desired FBO texture size. If it hasn't changed, this does nothing.
    // Otherwise, it must rebuild the FBOs
    QSize renderSize;
    if (OculusManager::isConnected()) {
        renderSize = OculusManager::getRenderTargetSize();
    } else {
        renderSize = _glWidget->getDeviceSize() * getRenderResolutionScale();
    }

    if (_renderResolution != toGlm(renderSize)) {
        _renderResolution = toGlm(renderSize);
        DependencyManager::get<TextureCache>()->setFrameBufferSize(renderSize);

        glViewport(0, 0, _renderResolution.x, _renderResolution.y); // shouldn't this account for the menu???

        updateProjectionMatrix();
        glLoadIdentity();
    }

    resetCameras(_myCamera, _renderResolution);

    auto offscreenUi = DependencyManager::get<OffscreenUi>();

    auto canvasSize = _glWidget->size();
    offscreenUi->resize(canvasSize);
    _glWidget->makeCurrent();

}

void Application::updateProjectionMatrix() {
    updateProjectionMatrix(_myCamera);
}

void Application::updateProjectionMatrix(Camera& camera, bool updateViewFrustum) {
    _projectionMatrix = camera.getProjection();

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(_projectionMatrix));

    // Tell our viewFrustum about this change, using the application camera
    if (updateViewFrustum) {
        loadViewFrustum(camera, _viewFrustum);
    }

    glMatrixMode(GL_MODELVIEW);
}

void Application::controlledBroadcastToNodes(const QByteArray& packet, const NodeSet& destinationNodeTypes) {
    foreach(NodeType_t type, destinationNodeTypes) {
        // Perform the broadcast for one type
        DependencyManager::get<NodeList>()->broadcastToNodes(packet, NodeSet() << type);
    }
}

bool Application::importSVOFromURL(const QString& urlString) {
    QUrl url(urlString);
    emit svoImportRequested(url.url());
    return true; // assume it's accepted
}

bool Application::event(QEvent* event) {
    if ((int)event->type() == (int)Lambda) {
        ((LambdaEvent*)event)->call();
        return true;
    }

    switch (event->type()) {
        case QEvent::MouseMove:
            mouseMoveEvent((QMouseEvent*)event);
            return true;
        case QEvent::MouseButtonPress:
            mousePressEvent((QMouseEvent*)event);
            return true;
        case QEvent::MouseButtonDblClick:
            mouseDoublePressEvent((QMouseEvent*)event);
            return true;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent((QMouseEvent*)event);
            return true;
        case QEvent::KeyPress:
            keyPressEvent((QKeyEvent*)event);
            return true;
        case QEvent::KeyRelease:
            keyReleaseEvent((QKeyEvent*)event);
            return true;
        case QEvent::FocusOut:
            focusOutEvent((QFocusEvent*)event);
            return true;
        case QEvent::TouchBegin:
            touchBeginEvent(static_cast<QTouchEvent*>(event));
            event->accept();
            return true;
        case QEvent::TouchEnd:
            touchEndEvent(static_cast<QTouchEvent*>(event));
            return true;
        case QEvent::TouchUpdate:
            touchUpdateEvent(static_cast<QTouchEvent*>(event));
            return true;
        case QEvent::Wheel:
            wheelEvent(static_cast<QWheelEvent*>(event));
            return true;
        case QEvent::Drop:
            dropEvent(static_cast<QDropEvent*>(event));
            return true;
        default:
            break;
    }

    // handle custom URL
    if (event->type() == QEvent::FileOpen) {

        QFileOpenEvent* fileEvent = static_cast<QFileOpenEvent*>(event);

        QUrl url = fileEvent->url();

        if (!url.isEmpty()) {
            QString urlString = url.toString();
            if (canAcceptURL(urlString)) {
                return acceptURL(urlString);
            }
        }
        return false;
    }

    if (HFActionEvent::types().contains(event->type())) {
        _controllerScriptingInterface.handleMetaEvent(static_cast<HFMetaEvent*>(event));
    }

    return QApplication::event(event);
}

bool Application::eventFilter(QObject* object, QEvent* event) {

    if (event->type() == QEvent::ShortcutOverride) {
        if (DependencyManager::get<OffscreenUi>()->shouldSwallowShortcut(event)) {
            event->accept();
            return true;
        }

        // Filter out captured keys before they're used for shortcut actions.
        if (_controllerScriptingInterface.isKeyCaptured(static_cast<QKeyEvent*>(event))) {
            event->accept();
            return true;
        }
    }

    return false;
}

static bool _altPressed{ false };

void Application::keyPressEvent(QKeyEvent* event) {
    _altPressed = event->key() == Qt::Key_Alt;
    _keysPressed.insert(event->key());

    _controllerScriptingInterface.emitKeyPressEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isKeyCaptured(event)) {
        return;
    }

    if (activeWindow() == _window) {
        _keyboardMouseDevice.keyPressEvent(event);

        bool isShifted = event->modifiers().testFlag(Qt::ShiftModifier);
        bool isMeta = event->modifiers().testFlag(Qt::ControlModifier);
        bool isOption = event->modifiers().testFlag(Qt::AltModifier);
        switch (event->key()) {
                break;
            case Qt::Key_Enter:
            case Qt::Key_Return:
                Menu::getInstance()->triggerOption(MenuOption::AddressBar);
                break;

            case Qt::Key_B:
                if (isMeta) {
                    auto offscreenUi = DependencyManager::get<OffscreenUi>();
                    offscreenUi->load("Browser.qml");
                }
                break;

            case Qt::Key_L:
                if (isShifted && isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::Log);
                } else if (isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::AddressBar);
                } else if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::LodTools);
                }
                break;

            case Qt::Key_F: {
                _physicsEngine.dumpNextStats();
                break;
            }

            case Qt::Key_Asterisk:
                Menu::getInstance()->triggerOption(MenuOption::Stars);
                break;

            case Qt::Key_W:
                if (isOption && !isShifted && !isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::Wireframe);
                }
                break;

            case Qt::Key_S:
                if (isShifted && isMeta && !isOption) {
                    Menu::getInstance()->triggerOption(MenuOption::SuppressShortTimings);
                } else if (isOption && !isShifted && !isMeta) {
                    Menu::getInstance()->triggerOption(MenuOption::ScriptEditor);
                } else if (!isOption && !isShifted && isMeta) {
                    takeSnapshot();
                }
                break;

            case Qt::Key_Apostrophe: {
                if (isMeta) {
                    auto cursor = Cursor::Manager::instance().getCursor();
                    auto curIcon = cursor->getIcon();
                    if (curIcon == Cursor::Icon::DEFAULT) {
                        cursor->setIcon(Cursor::Icon::LINK);
                    } else {
                        cursor->setIcon(Cursor::Icon::DEFAULT);
                    }
                } else {
                    resetSensors();
                }
                break;
            }

            case Qt::Key_A:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Atmosphere);
                }
                break;

            case Qt::Key_Backslash:
                Menu::getInstance()->triggerOption(MenuOption::Chat);
                break;

            case Qt::Key_Up:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    if (!isShifted) {
                        _scaleMirror *= 0.95f;
                    } else {
                        _raiseMirror += 0.05f;
                    }
                }
                break;

            case Qt::Key_Down:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    if (!isShifted) {
                        _scaleMirror *= 1.05f;
                    } else {
                        _raiseMirror -= 0.05f;
                    }
                }
                break;

            case Qt::Key_Left:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    _rotateMirror += PI / 20.0f;
                }
                break;

            case Qt::Key_Right:
                if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
                    _rotateMirror -= PI / 20.0f;
                }
                break;

#if 0
            case Qt::Key_I:
                if (isShifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0.002f, 0, 0)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0.001, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_K:
                if (isShifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(-0.002f, 0, 0)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, -0.001, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_J:
                if (isShifted) {
                    _viewFrustum.setFocalLength(_viewFrustum.getFocalLength() - 0.1f);
                    if (TV3DManager::isConnected()) {
                        TV3DManager::configureCamera(_myCamera, _glWidget->getDeviceWidth(), _glWidget->getDeviceHeight());
                    }
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(-0.001, 0, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_M:
                if (isShifted) {
                    _viewFrustum.setFocalLength(_viewFrustum.getFocalLength() + 0.1f);
                    if (TV3DManager::isConnected()) {
                        TV3DManager::configureCamera(_myCamera, _glWidget->getDeviceWidth(), _glWidget->getDeviceHeight());
                    }

                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0.001, 0, 0));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_U:
                if (isShifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0, 0, -0.002f)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0, -0.001));
                }
                updateProjectionMatrix();
                break;

            case Qt::Key_Y:
                if (isShifted) {
                    _myCamera.setEyeOffsetOrientation(glm::normalize(
                                                                     glm::quat(glm::vec3(0, 0, 0.002f)) * _myCamera.getEyeOffsetOrientation()));
                } else {
                    _myCamera.setEyeOffsetPosition(_myCamera.getEyeOffsetPosition() + glm::vec3(0, 0, 0.001));
                }
                updateProjectionMatrix();
                break;
#endif

            case Qt::Key_H:
                if (isShifted) {
                    Menu::getInstance()->triggerOption(MenuOption::Mirror);
                } else {
                    Menu::getInstance()->setIsOptionChecked(MenuOption::FullscreenMirror, !Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror));
                    if (!Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
                        Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, true);
                    }
                    cameraMenuChanged();
                }
                break;
            case Qt::Key_P:
                 Menu::getInstance()->setIsOptionChecked(MenuOption::FirstPerson, !Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson));
                 Menu::getInstance()->setIsOptionChecked(MenuOption::ThirdPerson, !Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson));
                 cameraMenuChanged();
                 break;
            case Qt::Key_Slash:
                Menu::getInstance()->triggerOption(MenuOption::Stats);
                break;

            case Qt::Key_Plus: {
                if (isMeta && event->modifiers().testFlag(Qt::KeypadModifier)) {
                    auto& cursorManager = Cursor::Manager::instance();
                    cursorManager.setScale(cursorManager.getScale() * 1.1f);
                } else {
                    _myAvatar->increaseSize();
                }
                break;
            }

            case Qt::Key_Minus: {
                if (isMeta && event->modifiers().testFlag(Qt::KeypadModifier)) {
                    auto& cursorManager = Cursor::Manager::instance();
                    cursorManager.setScale(cursorManager.getScale() / 1.1f);
                } else {
                    _myAvatar->decreaseSize();
                }
                break;
            }

            case Qt::Key_Equal:
                _myAvatar->resetSize();
                break;
            case Qt::Key_Space: {
                if (!event->isAutoRepeat()) {
                    // this starts an HFActionEvent
                    HFActionEvent startActionEvent(HFActionEvent::startType(),
                                                   computePickRay(getTrueMouseX(), getTrueMouseY()));
                    sendEvent(this, &startActionEvent);
                }

                break;
            }
            case Qt::Key_Escape: {
                OculusManager::abandonCalibration();

                if (!event->isAutoRepeat()) {
                    // this starts the HFCancelEvent
                    HFBackEvent startBackEvent(HFBackEvent::startType());
                    sendEvent(this, &startBackEvent);
                }

                break;
            }

            default:
                event->ignore();
                break;
        }
    }
}


#define VR_MENU_ONLY_IN_HMD

void Application::keyReleaseEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Alt && _altPressed && _window->isActiveWindow()) {
#ifdef VR_MENU_ONLY_IN_HMD
        if (isHMDMode()) {
#endif
            VrMenu::toggle();
#ifdef VR_MENU_ONLY_IN_HMD
        }
#endif
    }

    _keysPressed.remove(event->key());

    _controllerScriptingInterface.emitKeyReleaseEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isKeyCaptured(event)) {
        return;
    }

    _keyboardMouseDevice.keyReleaseEvent(event);

    switch (event->key()) {
        case Qt::Key_Space: {
            if (!event->isAutoRepeat()) {
                // this ends the HFActionEvent
                HFActionEvent endActionEvent(HFActionEvent::endType(),
                                             computePickRay(getTrueMouseX(), getTrueMouseY()));
                sendEvent(this, &endActionEvent);
            }
            break;
        }
        case Qt::Key_Escape: {
            if (!event->isAutoRepeat()) {
                // this ends the HFCancelEvent
                HFBackEvent endBackEvent(HFBackEvent::endType());
                sendEvent(this, &endBackEvent);
            }
            break;
        }
        default:
            event->ignore();
            break;
    }
}

void Application::focusOutEvent(QFocusEvent* event) {
    _keyboardMouseDevice.focusOutEvent(event);
    SixenseManager::getInstance().focusOutEvent();
    SDL2Manager::getInstance()->focusOutEvent();

    // synthesize events for keys currently pressed, since we may not get their release events
    foreach (int key, _keysPressed) {
        QKeyEvent event(QEvent::KeyRelease, key, Qt::NoModifier);
        keyReleaseEvent(&event);
    }
    _keysPressed.clear();
}

void Application::mouseMoveEvent(QMouseEvent* event, unsigned int deviceID) {
    // Used by application overlay to determine how to draw cursor(s)
    _lastMouseMoveWasSimulated = deviceID > 0;
    if (!_lastMouseMoveWasSimulated) {
        _lastMouseMove = usecTimestampNow();
    }

    if (_aboutToQuit) {
        return;
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::Fullscreen)
        && !Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode)) {
        // Show/hide menu bar in fullscreen
        if (event->globalY() > _menuBarHeight) {
            _fullscreenMenuWidget->setFixedHeight(0);
            Menu::getInstance()->setFixedHeight(0);
        } else {
            _fullscreenMenuWidget->setFixedHeight(_menuBarHeight);
            Menu::getInstance()->setFixedHeight(_menuBarHeight);
        }
    }

    _entities.mouseMoveEvent(event, deviceID);

    _controllerScriptingInterface.emitMouseMoveEvent(event, deviceID); // send events to any registered scripts
    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    _keyboardMouseDevice.mouseMoveEvent(event, deviceID);

}

void Application::mousePressEvent(QMouseEvent* event, unsigned int deviceID) {
    // Inhibit the menu if the user is using alt-mouse dragging
    _altPressed = false;

    if (!_aboutToQuit) {
        _entities.mousePressEvent(event, deviceID);
    }

    _controllerScriptingInterface.emitMousePressEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }


    if (activeWindow() == _window) {
        _keyboardMouseDevice.mousePressEvent(event);

        if (event->button() == Qt::LeftButton) {
            _mouseDragStarted = getTrueMouse();
            _mousePressed = true;

            // nobody handled this - make it an action event on the _window object
            HFActionEvent actionEvent(HFActionEvent::startType(),
                                      computePickRay(event->x(), event->y()));
            sendEvent(this, &actionEvent);

        } else if (event->button() == Qt::RightButton) {
            // right click items here
        }
    }
}

void Application::mouseDoublePressEvent(QMouseEvent* event, unsigned int deviceID) {
    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    _controllerScriptingInterface.emitMouseDoublePressEvent(event);
}

void Application::mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID) {

    if (!_aboutToQuit) {
        _entities.mouseReleaseEvent(event, deviceID);
    }

    _controllerScriptingInterface.emitMouseReleaseEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isMouseCaptured()) {
        return;
    }

    if (activeWindow() == _window) {
        _keyboardMouseDevice.mouseReleaseEvent(event);

        if (event->button() == Qt::LeftButton) {
            _mousePressed = false;

            // fire an action end event
            HFActionEvent actionEvent(HFActionEvent::endType(),
                                      computePickRay(event->x(), event->y()));
            sendEvent(this, &actionEvent);
        }
    }
}

void Application::touchUpdateEvent(QTouchEvent* event) {
    _altPressed = false;

    if (event->type() == QEvent::TouchUpdate) {
        TouchEvent thisEvent(*event, _lastTouchEvent);
        _controllerScriptingInterface.emitTouchUpdateEvent(thisEvent); // send events to any registered scripts
        _lastTouchEvent = thisEvent;
    }

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }

    _keyboardMouseDevice.touchUpdateEvent(event);

    bool validTouch = false;
    if (activeWindow() == _window) {
        const QList<QTouchEvent::TouchPoint>& tPoints = event->touchPoints();
        _touchAvg = vec2();
        int numTouches = tPoints.count();
        if (numTouches > 1) {
            for (int i = 0; i < numTouches; ++i) {
                _touchAvg += toGlm(tPoints[i].pos());
            }
            _touchAvg /= (float)(numTouches);
            validTouch = true;
        }
    }
    if (!_isTouchPressed) {
        _touchDragStartedAvg = _touchAvg;
    }
    _isTouchPressed = validTouch;
}

void Application::touchBeginEvent(QTouchEvent* event) {
    _altPressed = false;
    TouchEvent thisEvent(*event); // on touch begin, we don't compare to last event
    _controllerScriptingInterface.emitTouchBeginEvent(thisEvent); // send events to any registered scripts

    _lastTouchEvent = thisEvent; // and we reset our last event to this event before we call our update
    touchUpdateEvent(event);

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }

    _keyboardMouseDevice.touchBeginEvent(event);

}

void Application::touchEndEvent(QTouchEvent* event) {
    _altPressed = false;
    TouchEvent thisEvent(*event, _lastTouchEvent);
    _controllerScriptingInterface.emitTouchEndEvent(thisEvent); // send events to any registered scripts
    _lastTouchEvent = thisEvent;

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isTouchCaptured()) {
        return;
    }

    _keyboardMouseDevice.touchEndEvent(event);

    // put any application specific touch behavior below here..
    _touchDragStartedAvg = _touchAvg;
    _isTouchPressed = false;

}

void Application::wheelEvent(QWheelEvent* event) {
    _altPressed = false;
    _controllerScriptingInterface.emitWheelEvent(event); // send events to any registered scripts

    // if one of our scripts have asked to capture this event, then stop processing it
    if (_controllerScriptingInterface.isWheelCaptured()) {
        return;
    }

    _keyboardMouseDevice.wheelEvent(event);
}

void Application::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    bool atLeastOneFileAccepted = false;
    foreach (QUrl url, mimeData->urls()) {
        QString urlString = url.toString();
        if (canAcceptURL(urlString)) {
            if (acceptURL(urlString)) {
                atLeastOneFileAccepted = true;
                break;
            }
        }
    }

    if (atLeastOneFileAccepted) {
        event->acceptProposedAction();
    }
}

void Application::dragEnterEvent(QDragEnterEvent* event) {
    const QMimeData* mimeData = event->mimeData();
    foreach(QUrl url, mimeData->urls()) {
        auto urlString = url.toString();
        if (canAcceptURL(urlString)) {
            event->acceptProposedAction();
            break;
        }
    }
}

bool Application::acceptSnapshot(const QString& urlString) {
    QUrl url(urlString);
    QString snapshotPath = url.toLocalFile();

    SnapshotMetaData* snapshotData = Snapshot::parseSnapshotData(snapshotPath);
    if (snapshotData) {
        if (!snapshotData->getURL().toString().isEmpty()) {
            DependencyManager::get<AddressManager>()->handleLookupString(snapshotData->getURL().toString());
        }
    } else {
        QMessageBox msgBox;
        msgBox.setText("No location details were found in the file "
                        + snapshotPath + ", try dragging in an authentic Hifi snapshot.");

        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
    return true;
}

void Application::sendPingPackets() {
    QByteArray pingPacket = DependencyManager::get<NodeList>()->constructPingPacket();
    controlledBroadcastToNodes(pingPacket, NodeSet()
                               << NodeType::EntityServer
                               << NodeType::AudioMixer << NodeType::AvatarMixer);
}

//  Every second, check the frame rates and other stuff
void Application::checkFPS() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::TestPing)) {
        sendPingPackets();
    }

    float diffTime = (float)_timerStart.nsecsElapsed() / 1000000000.0f;

    _fps = (float)_frameCount / diffTime;
    _frameCount = 0;
    _datagramProcessor->resetCounters();
    _timerStart.start();

    // ask the node list to check in with the domain server
    DependencyManager::get<NodeList>()->sendDomainServerCheckIn();
}

void Application::idle() {
    static SimpleAverage<float> interIdleDurations;
    static uint64_t lastIdleEnd{ 0 };

    if (lastIdleEnd != 0) {
        uint64_t now = usecTimestampNow();
        interIdleDurations.update(now - lastIdleEnd);
        static uint64_t lastReportTime = now;
        if ((now - lastReportTime) >= (USECS_PER_SECOND)) {
            static QString LOGLINE("Average inter-idle time: %1 us for %2 samples");
            if (Menu::getInstance()->isOptionChecked(MenuOption::LogExtraTimings)) {
                qCDebug(interfaceapp_timing) << LOGLINE.arg((int)interIdleDurations.getAverage()).arg(interIdleDurations.getCount());
            }
            interIdleDurations.reset();
            lastReportTime = now;
        }
    }

    PerformanceTimer perfTimer("idle");
    if (_aboutToQuit) {
        return; // bail early, nothing to do here.
    }

    // Normally we check PipelineWarnings, but since idle will often take more than 10ms we only show these idle timing
    // details if we're in ExtraDebugging mode. However, the ::update() and its subcomponents will show their timing
    // details normally.
    bool showWarnings = getLogger()->extraDebugging();
    PerformanceWarning warn(showWarnings, "idle()");

    //  Only run simulation code if more than the targetFramePeriod have passed since last time we ran
    double targetFramePeriod = 0.0;
    unsigned int targetFramerate = getRenderTargetFramerate();
    if (targetFramerate > 0) {
        targetFramePeriod = 1000.0 / targetFramerate;
    }
    double timeSinceLastUpdate = (double)_lastTimeUpdated.nsecsElapsed() / 1000000.0;
    if (timeSinceLastUpdate > targetFramePeriod) {
        _lastTimeUpdated.start();
        {
            PerformanceTimer perfTimer("update");
            PerformanceWarning warn(showWarnings, "Application::idle()... update()");
            const float BIGGEST_DELTA_TIME_SECS = 0.25f;
            update(glm::clamp((float)timeSinceLastUpdate / 1000.0f, 0.0f, BIGGEST_DELTA_TIME_SECS));
        }
        {
            PerformanceTimer perfTimer("updateGL");
            PerformanceWarning warn(showWarnings, "Application::idle()... updateGL()");
            _glWidget->updateGL();
        }
        {
            PerformanceTimer perfTimer("rest");
            PerformanceWarning warn(showWarnings, "Application::idle()... rest of it");
            _idleLoopStdev.addValue(timeSinceLastUpdate);

            //  Record standard deviation and reset counter if needed
            const int STDEV_SAMPLES = 500;
            if (_idleLoopStdev.getSamples() > STDEV_SAMPLES) {
                _idleLoopMeasuredJitter = _idleLoopStdev.getStDev();
                _idleLoopStdev.reset();
            }

        }
        // After finishing all of the above work, ensure the idle timer is set to the proper interval,
        // depending on whether we're throttling or not
        idleTimer->start(_glWidget->isThrottleRendering() ? THROTTLED_IDLE_TIMER_DELAY : 0);
    } 

    // check for any requested background downloads.
    emit checkBackgroundDownloads();
    lastIdleEnd = usecTimestampNow();
}

void Application::setFullscreen(bool fullscreen) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::Fullscreen) != fullscreen) {
        Menu::getInstance()->getActionForOption(MenuOption::Fullscreen)->setChecked(fullscreen);
    }

    // The following code block is useful on platforms that can have a visible
    // app menu in a fullscreen window.  However the OSX mechanism hides the
    // application menu for fullscreen apps, so the check is not required.
#ifndef Q_OS_MAC
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode)) {
        if (fullscreen) {
            // Menu hide() disables menu commands, and show() after hide() doesn't work with Rift VR display.
            // So set height instead.
            _window->menuBar()->setMaximumHeight(0);
        } else {
            _window->menuBar()->setMaximumHeight(QWIDGETSIZE_MAX);
        }
    } else {
        if (fullscreen) {
            // Move menu to a QWidget floating above _glWidget so that show/hide doesn't adjust viewport.
            _menuBarHeight = Menu::getInstance()->height();
            Menu::getInstance()->setParent(_fullscreenMenuWidget);
            Menu::getInstance()->setFixedWidth(_window->windowHandle()->screen()->size().width());
            _fullscreenMenuWidget->show();
        } else {
            // Restore menu to being part of MainWindow.
            _fullscreenMenuWidget->hide();
            _window->setMenuBar(Menu::getInstance());
            _window->menuBar()->setMaximumHeight(QWIDGETSIZE_MAX);
        }
    }
#endif

    // Work around Qt bug that prevents floating menus being shown when in fullscreen mode.
    // https://bugreports.qt.io/browse/QTBUG-41883
    // Known issue: Top-level menu items don't highlight when cursor hovers. This is probably a side-effect of the work-around.
    // TODO: Remove this work-around once the bug has been fixed and restore the following lines.
    //_window->setWindowState(fullscreen ? (_window->windowState() | Qt::WindowFullScreen) :
    //    (_window->windowState() & ~Qt::WindowFullScreen));
    _window->hide();
    if (fullscreen) {
        _window->setWindowState(_window->windowState() | Qt::WindowFullScreen);
        // The next line produces the following warning in the log:
        // [WARNING][03 / 06 12:17 : 58] QWidget::setMinimumSize: (/ MainWindow) Negative sizes
        //   (0, -1) are not possible
        // This is better than the alternative which is to have the window slightly less than fullscreen with a visible line
        // of pixels for the remainder of the screen.
        _window->setContentsMargins(0, 0, 0, -1);
    } else {
        _window->setWindowState(_window->windowState() & ~Qt::WindowFullScreen);
        _window->setContentsMargins(0, 0, 0, 0);
    }

    if (!_aboutToQuit) {
        _window->show();
    }
}

void Application::setEnable3DTVMode(bool enable3DTVMode) {
    resizeGL();
}

void Application::setEnableVRMode(bool enableVRMode) {
    if (Menu::getInstance()->isOptionChecked(MenuOption::EnableVRMode) != enableVRMode) {
        Menu::getInstance()->getActionForOption(MenuOption::EnableVRMode)->setChecked(enableVRMode);
    }

    if (enableVRMode) {
        if (!OculusManager::isConnected()) {
            // attempt to reconnect the Oculus manager - it's possible this was a workaround
            // for the sixense crash
            OculusManager::disconnect();
            OculusManager::connect(_glWidget->context()->contextHandle());
            _glWidget->setFocus();
            _glWidget->makeCurrent();
            glClear(GL_COLOR_BUFFER_BIT);
        }
        OculusManager::recalibrate();
    } else {
        OculusManager::abandonCalibration();
        OculusManager::disconnect();
    }

    resizeGL();
}

void Application::setLowVelocityFilter(bool lowVelocityFilter) {
    SixenseManager::getInstance().setLowVelocityFilter(lowVelocityFilter);
}

bool Application::mouseOnScreen() const {
    if (OculusManager::isConnected()) {
        ivec2 mouse = getMouse();
        if (!glm::all(glm::greaterThanEqual(mouse, ivec2()))) {
            return false;
        }
        ivec2 size = toGlm(_glWidget->getDeviceSize());
        if (!glm::all(glm::lessThanEqual(mouse, size))) {
            return false;
        }
    }
    return true;
}

ivec2 Application::getMouse() const {
    if (OculusManager::isConnected()) {
        return _compositor.screenToOverlay(getTrueMouse());
    }
    return getTrueMouse();
}

ivec2 Application::getMouseDragStarted() const {
    if (OculusManager::isConnected()) {
        return _compositor.screenToOverlay(getTrueMouseDragStarted());
    }
    return getTrueMouseDragStarted();
}

ivec2 Application::getTrueMouseDragStarted() const {
    return _mouseDragStarted;
}

FaceTracker* Application::getActiveFaceTracker() {
    auto faceshift = DependencyManager::get<Faceshift>();
    auto dde = DependencyManager::get<DdeFaceTracker>();

    return (dde->isActive() ? static_cast<FaceTracker*>(dde.data()) :
            (faceshift->isActive() ? static_cast<FaceTracker*>(faceshift.data()) : NULL));
}

FaceTracker* Application::getSelectedFaceTracker() {
    FaceTracker* faceTracker = NULL;
#ifdef HAVE_FACESHIFT
    if (Menu::getInstance()->isOptionChecked(MenuOption::Faceshift)) {
        faceTracker = DependencyManager::get<Faceshift>().data();
    }
#endif
#ifdef HAVE_DDE
    if (Menu::getInstance()->isOptionChecked(MenuOption::UseCamera)) {
        faceTracker = DependencyManager::get<DdeFaceTracker>().data();
    }
#endif
    return faceTracker;
}

void Application::setActiveFaceTracker() {
#if defined(HAVE_FACESHIFT) || defined(HAVE_DDE)
    bool isMuted = Menu::getInstance()->isOptionChecked(MenuOption::MuteFaceTracking);
#endif
#ifdef HAVE_FACESHIFT
    auto faceshiftTracker = DependencyManager::get<Faceshift>();
    faceshiftTracker->setIsMuted(isMuted);
    faceshiftTracker->setEnabled(Menu::getInstance()->isOptionChecked(MenuOption::Faceshift) && !isMuted);
#endif
#ifdef HAVE_DDE
    bool isUsingDDE = Menu::getInstance()->isOptionChecked(MenuOption::UseCamera);
    Menu::getInstance()->getActionForOption(MenuOption::BinaryEyelidControl)->setVisible(isUsingDDE);
    Menu::getInstance()->getActionForOption(MenuOption::UseAudioForMouth)->setVisible(isUsingDDE);
    Menu::getInstance()->getActionForOption(MenuOption::VelocityFilter)->setVisible(isUsingDDE);
    Menu::getInstance()->getActionForOption(MenuOption::CalibrateCamera)->setVisible(isUsingDDE);
    auto ddeTracker = DependencyManager::get<DdeFaceTracker>();
    ddeTracker->setIsMuted(isMuted);
    ddeTracker->setEnabled(isUsingDDE && !isMuted);
#endif
}

void Application::toggleFaceTrackerMute() {
    FaceTracker* faceTracker = getSelectedFaceTracker();
    if (faceTracker) {
        faceTracker->toggleMute();
    }
}

bool Application::exportEntities(const QString& filename, const QVector<EntityItemID>& entityIDs) {
    QVector<EntityItemPointer> entities;

    auto entityTree = _entities.getTree();
    EntityTree exportTree;

    glm::vec3 root(TREE_SCALE, TREE_SCALE, TREE_SCALE);
    for (auto entityID : entityIDs) {
        auto entityItem = entityTree->findEntityByEntityItemID(entityID);
        if (!entityItem) {
            continue;
        }

        auto properties = entityItem->getProperties();
        auto position = properties.getPosition();

        root.x = glm::min(root.x, position.x);
        root.y = glm::min(root.y, position.y);
        root.z = glm::min(root.z, position.z);

        entities << entityItem;
    }

    if (entities.size() == 0) {
        return false;
    }

    for (auto entityItem : entities) {
        auto properties = entityItem->getProperties();

        properties.setPosition(properties.getPosition() - root);
        exportTree.addEntity(entityItem->getEntityItemID(), properties);
    }

    exportTree.writeToJSONFile(filename.toLocal8Bit().constData());

    // restore the main window's active state
    _window->activateWindow();
    return true;
}

bool Application::exportEntities(const QString& filename, float x, float y, float z, float scale) {
    QVector<EntityItemPointer> entities;
    _entities.getTree()->findEntities(AACube(glm::vec3(x, y, z), scale), entities);

    if (entities.size() > 0) {
        glm::vec3 root(x, y, z);
        EntityTree exportTree;

        for (int i = 0; i < entities.size(); i++) {
            EntityItemProperties properties = entities.at(i)->getProperties();
            EntityItemID id = entities.at(i)->getEntityItemID();
            properties.setPosition(properties.getPosition() - root);
            exportTree.addEntity(id, properties);
        }
        exportTree.writeToSVOFile(filename.toLocal8Bit().constData());
    } else {
        qCDebug(interfaceapp) << "No models were selected";
        return false;
    }

    // restore the main window's active state
    _window->activateWindow();
    return true;
}

void Application::loadSettings() {

    DependencyManager::get<AudioClient>()->loadSettings();
    DependencyManager::get<LODManager>()->loadSettings();

    Menu::getInstance()->loadSettings();
    _myAvatar->loadData();
}

void Application::saveSettings() {
    DependencyManager::get<AudioClient>()->saveSettings();
    DependencyManager::get<LODManager>()->saveSettings();

    Menu::getInstance()->saveSettings();
    _myAvatar->saveData();
}

bool Application::importEntities(const QString& urlOrFilename) {
    _entityClipboard.eraseAllOctreeElements();

    QUrl url(urlOrFilename);

    // if the URL appears to be invalid or relative, then it is probably a local file
    if (!url.isValid() || url.isRelative()) {
        url = QUrl::fromLocalFile(urlOrFilename);
    }

    bool success = _entityClipboard.readFromURL(url.toString());
    if (success) {
        _entityClipboard.reaverageOctreeElements();
    }
    return success;
}

QVector<EntityItemID> Application::pasteEntities(float x, float y, float z) {
    return _entityClipboard.sendEntities(&_entityEditSender, _entities.getTree(), x, y, z);
}

void Application::initDisplay() {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_CONSTANT_ALPHA, GL_ONE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST);
}

void Application::init() {
    // Make sure Login state is up to date
    DependencyManager::get<DialogsManager>()->toggleLoginDialog();

    _environment.init();

    DependencyManager::get<DeferredLightingEffect>()->init(this);
    DependencyManager::get<AmbientOcclusionEffect>()->init(this);

    // TODO: move _myAvatar out of Application. Move relevant code to MyAvataar or AvatarManager
    DependencyManager::get<AvatarManager>()->init();
    _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);

    _mirrorCamera.setMode(CAMERA_MODE_MIRROR);

    TV3DManager::connect();
    if (TV3DManager::isConnected()) {
        QMetaObject::invokeMethod(Menu::getInstance()->getActionForOption(MenuOption::Fullscreen),
                                  "trigger",
                                  Qt::QueuedConnection);
    }

    _timerStart.start();
    _lastTimeUpdated.start();

    // when --url in command line, teleport to location
    const QString HIFI_URL_COMMAND_LINE_KEY = "--url";
    int urlIndex = arguments().indexOf(HIFI_URL_COMMAND_LINE_KEY);
    QString addressLookupString;
    if (urlIndex != -1) {
        addressLookupString = arguments().value(urlIndex + 1);
    }

    DependencyManager::get<AddressManager>()->loadSettings(addressLookupString);

    qCDebug(interfaceapp) << "Loaded settings";

#ifdef __APPLE__
    if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseEnabled)) {
        // on OS X we only setup sixense if the user wants it on - this allows running without the hid_init crash
        // if hydra support is temporarily not required
        SixenseManager::getInstance().toggleSixense(true);
    }
#else
    // setup sixense
    SixenseManager::getInstance().toggleSixense(true);
#endif

    Leapmotion::init();
    RealSense::init();

    // fire off an immediate domain-server check in now that settings are loaded
    DependencyManager::get<NodeList>()->sendDomainServerCheckIn();

    _entities.init();
    _entities.setViewFrustum(getViewFrustum());

    ObjectMotionState::setShapeManager(&_shapeManager);
    _physicsEngine.init();

    EntityTree* tree = _entities.getTree();
    _entitySimulation.init(tree, &_physicsEngine, &_entityEditSender);
    tree->setSimulation(&_entitySimulation);

    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();

    // connect the _entityCollisionSystem to our EntityTreeRenderer since that's what handles running entity scripts
    connect(&_entitySimulation, &EntitySimulation::entityCollisionWithEntity,
            &_entities, &EntityTreeRenderer::entityCollisionWithEntity);

    // connect the _entities (EntityTreeRenderer) to our script engine's EntityScriptingInterface for firing
    // of events related clicking, hovering over, and entering entities
    _entities.connectSignalsToSlots(entityScriptingInterface.data());

    _entityClipboardRenderer.init();
    _entityClipboardRenderer.setViewFrustum(getViewFrustum());
    _entityClipboardRenderer.setTree(&_entityClipboard);

    // initialize the GlowEffect with our widget
    bool glow = Menu::getInstance()->isOptionChecked(MenuOption::EnableGlowEffect);
    DependencyManager::get<GlowEffect>()->init(glow);

    // Make sure any new sounds are loaded as soon as know about them.
    connect(tree, &EntityTree::newCollisionSoundURL, DependencyManager::get<SoundCache>().data(), &SoundCache::getSound);
    connect(_myAvatar, &MyAvatar::newCollisionSoundURL, DependencyManager::get<SoundCache>().data(), &SoundCache::getSound);
}

void Application::closeMirrorView() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::Mirror)) {
        Menu::getInstance()->triggerOption(MenuOption::Mirror);
    }
}

void Application::restoreMirrorView() {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        Menu::getInstance()->triggerOption(MenuOption::FullscreenMirror);
    }
}

void Application::shrinkMirrorView() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        Menu::getInstance()->triggerOption(MenuOption::FullscreenMirror);
    }
}

const float HEAD_SPHERE_RADIUS = 0.1f;

bool Application::isLookingAtMyAvatar(Avatar* avatar) {
    glm::vec3 theirLookAt = avatar->getHead()->getLookAtPosition();
    glm::vec3 myEyePosition = _myAvatar->getHead()->getEyePosition();
    if (pointInSphere(theirLookAt, myEyePosition, HEAD_SPHERE_RADIUS * _myAvatar->getScale())) {
        return true;
    }
    return false;
}

void Application::updateLOD() {
    PerformanceTimer perfTimer("LOD");
    // adjust it unless we were asked to disable this feature, or if we're currently in throttleRendering mode
    if (!isThrottleRendering()) {
        DependencyManager::get<LODManager>()->autoAdjustLOD(_fps);
    } else {
        DependencyManager::get<LODManager>()->resetLODAdjust();
    }
}

void Application::updateMouseRay() {
    PerformanceTimer perfTimer("mouseRay");

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMouseRay()");

    // make sure the frustum is up-to-date
    loadViewFrustum(_myCamera, _viewFrustum);

    PickRay pickRay = computePickRay(getTrueMouseX(), getTrueMouseY());
    _mouseRayOrigin = pickRay.origin;
    _mouseRayDirection = pickRay.direction;

    // adjust for mirroring
    if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        glm::vec3 mouseRayOffset = _mouseRayOrigin - _viewFrustum.getPosition();
        _mouseRayOrigin -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), mouseRayOffset) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), mouseRayOffset));
        _mouseRayDirection -= 2.0f * (_viewFrustum.getDirection() * glm::dot(_viewFrustum.getDirection(), _mouseRayDirection) +
            _viewFrustum.getRight() * glm::dot(_viewFrustum.getRight(), _mouseRayDirection));
    }
}

void Application::updateMyAvatarLookAtPosition() {
    PerformanceTimer perfTimer("lookAt");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateMyAvatarLookAtPosition()");

    _myAvatar->updateLookAtTargetAvatar();
    FaceTracker* tracker = getActiveFaceTracker();

    bool isLookingAtSomeone = false;
    glm::vec3 lookAtSpot;
    if (_myCamera.getMode() == CAMERA_MODE_MIRROR) {
        //  When I am in mirror mode, just look right at the camera (myself); don't switch gaze points because when physically
        //  looking in a mirror one's eyes appear steady.
        if (!OculusManager::isConnected()) {
            lookAtSpot = _myCamera.getPosition();
        } else {
            lookAtSpot = _myCamera.getPosition() + OculusManager::getMidEyePosition();
        }
    } else {
        AvatarSharedPointer lookingAt = _myAvatar->getLookAtTargetAvatar().lock();
        if (lookingAt && _myAvatar != lookingAt.get()) {
            //  If I am looking at someone else, look directly at one of their eyes
            isLookingAtSomeone = true;
            Head* lookingAtHead = static_cast<Avatar*>(lookingAt.get())->getHead();

            const float MAXIMUM_FACE_ANGLE = 65.0f * RADIANS_PER_DEGREE;
            glm::vec3 lookingAtFaceOrientation = lookingAtHead->getFinalOrientationInWorldFrame() * IDENTITY_FRONT;
            glm::vec3 fromLookingAtToMe = glm::normalize(_myAvatar->getHead()->getEyePosition()
                - lookingAtHead->getEyePosition());
            float faceAngle = glm::angle(lookingAtFaceOrientation, fromLookingAtToMe);

            if (faceAngle < MAXIMUM_FACE_ANGLE) {
                // Randomly look back and forth between look targets
                switch (_myAvatar->getEyeContactTarget()) {
                    case LEFT_EYE:
                        lookAtSpot = lookingAtHead->getLeftEyePosition();
                        break;
                    case RIGHT_EYE:
                        lookAtSpot = lookingAtHead->getRightEyePosition();
                        break;
                    case MOUTH:
                        lookAtSpot = lookingAtHead->getMouthPosition();
                        break;
                }
            } else {
                // Just look at their head (mid point between eyes)
                lookAtSpot = lookingAtHead->getEyePosition();
            }
        } else {
            //  I am not looking at anyone else, so just look forward
            lookAtSpot = _myAvatar->getHead()->getEyePosition() +
                (_myAvatar->getHead()->getFinalOrientationInWorldFrame() * glm::vec3(0.0f, 0.0f, -TREE_SCALE));
        }
    }

    // Deflect the eyes a bit to match the detected gaze from Faceshift if active.
    // DDE doesn't track eyes.
    if (tracker && typeid(*tracker) == typeid(Faceshift) && !tracker->isMuted()) {
        float eyePitch = tracker->getEstimatedEyePitch();
        float eyeYaw = tracker->getEstimatedEyeYaw();
        const float GAZE_DEFLECTION_REDUCTION_DURING_EYE_CONTACT = 0.1f;
        glm::vec3 origin = _myAvatar->getHead()->getEyePosition();
        float pitchSign = (_myCamera.getMode() == CAMERA_MODE_MIRROR) ? -1.0f : 1.0f;
        float deflection = DependencyManager::get<Faceshift>()->getEyeDeflection();
        if (isLookingAtSomeone) {
            deflection *= GAZE_DEFLECTION_REDUCTION_DURING_EYE_CONTACT;
        }
        lookAtSpot = origin + _myCamera.getRotation() * glm::quat(glm::radians(glm::vec3(
            eyePitch * pitchSign * deflection, eyeYaw * deflection, 0.0f))) *
                glm::inverse(_myCamera.getRotation()) * (lookAtSpot - origin);
    }

    _myAvatar->getHead()->setLookAtPosition(lookAtSpot);
}

void Application::updateThreads(float deltaTime) {
    PerformanceTimer perfTimer("updateThreads");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateThreads()");

    // parse voxel packets
    if (!_enableProcessOctreeThread) {
        _octreeProcessor.threadRoutine();
        _entityEditSender.threadRoutine();
    }
}

void Application::cameraMenuChanged() {
    if (Menu::getInstance()->isOptionChecked(MenuOption::FullscreenMirror)) {
        if (_myCamera.getMode() != CAMERA_MODE_MIRROR) {
            _myCamera.setMode(CAMERA_MODE_MIRROR);
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::FirstPerson)) {
        if (_myCamera.getMode() != CAMERA_MODE_FIRST_PERSON) {
            _myCamera.setMode(CAMERA_MODE_FIRST_PERSON);
            _myAvatar->setBoomLength(MyAvatar::ZOOM_MIN);
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::ThirdPerson)) {
        if (_myCamera.getMode() != CAMERA_MODE_THIRD_PERSON) {
            _myCamera.setMode(CAMERA_MODE_THIRD_PERSON);
            if (_myAvatar->getBoomLength() == MyAvatar::ZOOM_MIN) {
                _myAvatar->setBoomLength(MyAvatar::ZOOM_DEFAULT);
            }
        }
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::IndependentMode)) {
        if (_myCamera.getMode() != CAMERA_MODE_INDEPENDENT) {
            _myCamera.setMode(CAMERA_MODE_INDEPENDENT);
        }
    }
}

void Application::rotationModeChanged() {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::CenterPlayerInView)) {
        _myAvatar->setHeadPitch(0);
    }
}

void Application::updateCamera(float deltaTime) {
    PerformanceTimer perfTimer("updateCamera");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateCamera()");
}

void Application::updateDialogs(float deltaTime) {
    PerformanceTimer perfTimer("updateDialogs");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateDialogs()");
    auto dialogsManager = DependencyManager::get<DialogsManager>();

    // Update bandwidth dialog, if any
    BandwidthDialog* bandwidthDialog = dialogsManager->getBandwidthDialog();
    if (bandwidthDialog) {
        bandwidthDialog->update();
    }

    QPointer<OctreeStatsDialog> octreeStatsDialog = dialogsManager->getOctreeStatsDialog();
    if (octreeStatsDialog) {
        octreeStatsDialog->update();
    }
}

void Application::updateCursor(float deltaTime) {
    PerformanceTimer perfTimer("updateCursor");
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateCursor()");

    static QPoint lastMousePos = QPoint();
    _lastMouseMove = (lastMousePos == QCursor::pos()) ? _lastMouseMove : usecTimestampNow();
    lastMousePos = QCursor::pos();
}

void Application::setCursorVisible(bool visible) {
    _cursorVisible = visible;
}

void Application::update(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::update()");

    updateLOD();
    updateMouseRay(); // check what's under the mouse and update the mouse voxel

    {
        PerformanceTimer perfTimer("devices");
        DeviceTracker::updateAll();
        FaceTracker* tracker = getActiveFaceTracker();
        if (tracker && !tracker->isMuted()) {
            tracker->update(deltaTime);

            // Auto-mute microphone after losing face tracking?
            if (tracker->isTracking()) {
                _lastFaceTrackerUpdate = usecTimestampNow();
            } else {
                const quint64 MUTE_MICROPHONE_AFTER_USECS = 5000000;  //5 secs
                Menu* menu = Menu::getInstance();
                if (menu->isOptionChecked(MenuOption::AutoMuteAudio) && !menu->isOptionChecked(MenuOption::MuteAudio)) {
                    if (_lastFaceTrackerUpdate > 0
                        && ((usecTimestampNow() - _lastFaceTrackerUpdate) > MUTE_MICROPHONE_AFTER_USECS)) {
                        menu->triggerOption(MenuOption::MuteAudio);
                        _lastFaceTrackerUpdate = 0;
                    }
                } else {
                    _lastFaceTrackerUpdate = 0;
                }
            }
        } else {
            _lastFaceTrackerUpdate = 0;
        }

        SixenseManager::getInstance().update(deltaTime);
        SDL2Manager::getInstance()->update();
    }

    _userInputMapper.update(deltaTime);
    _keyboardMouseDevice.update();

    // Dispatch input events
    _controllerScriptingInterface.updateInputControllers();

    // Transfer the user inputs to the driveKeys
    _myAvatar->clearDriveKeys();
    if (_myCamera.getMode() != CAMERA_MODE_INDEPENDENT) {
        if (!_controllerScriptingInterface.areActionsCaptured()) {
            _myAvatar->setDriveKeys(FWD, _userInputMapper.getActionState(UserInputMapper::LONGITUDINAL_FORWARD));
            _myAvatar->setDriveKeys(BACK, _userInputMapper.getActionState(UserInputMapper::LONGITUDINAL_BACKWARD));
            _myAvatar->setDriveKeys(UP, _userInputMapper.getActionState(UserInputMapper::VERTICAL_UP));
            _myAvatar->setDriveKeys(DOWN, _userInputMapper.getActionState(UserInputMapper::VERTICAL_DOWN));
            _myAvatar->setDriveKeys(LEFT, _userInputMapper.getActionState(UserInputMapper::LATERAL_LEFT));
            _myAvatar->setDriveKeys(RIGHT, _userInputMapper.getActionState(UserInputMapper::LATERAL_RIGHT));
            _myAvatar->setDriveKeys(ROT_UP, _userInputMapper.getActionState(UserInputMapper::PITCH_UP));
            _myAvatar->setDriveKeys(ROT_DOWN, _userInputMapper.getActionState(UserInputMapper::PITCH_DOWN));
            _myAvatar->setDriveKeys(ROT_LEFT, _userInputMapper.getActionState(UserInputMapper::YAW_LEFT));
            _myAvatar->setDriveKeys(ROT_RIGHT, _userInputMapper.getActionState(UserInputMapper::YAW_RIGHT));
        }
        _myAvatar->setDriveKeys(BOOM_IN, _userInputMapper.getActionState(UserInputMapper::BOOM_IN));
        _myAvatar->setDriveKeys(BOOM_OUT, _userInputMapper.getActionState(UserInputMapper::BOOM_OUT));
    }

    updateThreads(deltaTime); // If running non-threaded, then give the threads some time to process...

    //loop through all the other avatars and simulate them...
    DependencyManager::get<AvatarManager>()->updateOtherAvatars(deltaTime);

    updateCamera(deltaTime); // handle various camera tweaks like off axis projection
    updateDialogs(deltaTime); // update various stats dialogs if present
    updateCursor(deltaTime); // Handle cursor updates

    {
        PerformanceTimer perfTimer("physics");
        _myAvatar->relayDriveKeysToCharacterController();

        _entitySimulation.lock();
        _physicsEngine.deleteObjects(_entitySimulation.getObjectsToDelete());
        _entitySimulation.unlock();

        _entities.getTree()->lockForWrite();
        _entitySimulation.lock();
        _physicsEngine.addObjects(_entitySimulation.getObjectsToAdd());
        _entitySimulation.unlock();
        _entities.getTree()->unlock();

        _entities.getTree()->lockForWrite();
        _entitySimulation.lock();
        _physicsEngine.changeObjects(_entitySimulation.getObjectsToChange());
        _entitySimulation.unlock();
        _entities.getTree()->unlock();

        _entitySimulation.lock();
        _entitySimulation.applyActionChanges();
        _entitySimulation.unlock();


        AvatarManager* avatarManager = DependencyManager::get<AvatarManager>().data();
        _physicsEngine.deleteObjects(avatarManager->getObjectsToDelete());
        _physicsEngine.addObjects(avatarManager->getObjectsToAdd());
        _physicsEngine.changeObjects(avatarManager->getObjectsToChange());

        _entities.getTree()->lockForWrite();
        _physicsEngine.stepSimulation();
        _entities.getTree()->unlock();

        if (_physicsEngine.hasOutgoingChanges()) {
            _entities.getTree()->lockForWrite();
            _entitySimulation.lock();
            _entitySimulation.handleOutgoingChanges(_physicsEngine.getOutgoingChanges(), _physicsEngine.getSessionID());
            _entitySimulation.unlock();
            _entities.getTree()->unlock();

            _entities.getTree()->lockForWrite();
            avatarManager->handleOutgoingChanges(_physicsEngine.getOutgoingChanges());
            _entities.getTree()->unlock();

            auto collisionEvents = _physicsEngine.getCollisionEvents();
            avatarManager->handleCollisionEvents(collisionEvents);

            _physicsEngine.dumpStatsIfNecessary();

            if (!_aboutToQuit) {
                PerformanceTimer perfTimer("entities");
                // Collision events (and their scripts) must not be handled when we're locked, above. (That would risk
                // deadlock.)
                _entitySimulation.handleCollisionEvents(collisionEvents);
                // NOTE: the _entities.update() call below will wait for lock
                // and will simulate entity motion (the EntityTree has been given an EntitySimulation).
                _entities.update(); // update the models...
            }
        }
    }

    {
        PerformanceTimer perfTimer("overlays");
        _overlays.update(deltaTime);
    }

    {
        PerformanceTimer perfTimer("myAvatar");
        updateMyAvatarLookAtPosition();
        // Sample hardware, update view frustum if needed, and send avatar data to mixer/nodes
        DependencyManager::get<AvatarManager>()->updateMyAvatar(deltaTime);
    }

    {
        PerformanceTimer perfTimer("emitSimulating");
        // let external parties know we're updating
        emit simulating(deltaTime);
    }

    // Update _viewFrustum with latest camera and view frustum data...
    // NOTE: we get this from the view frustum, to make it simpler, since the
    // loadViewFrumstum() method will get the correct details from the camera
    // We could optimize this to not actually load the viewFrustum, since we don't
    // actually need to calculate the view frustum planes to send these details
    // to the server.
    {
        PerformanceTimer perfTimer("loadViewFrustum");
        loadViewFrustum(_myCamera, _viewFrustum);
    }

    quint64 now = usecTimestampNow();

    // Update my voxel servers with my current voxel query...
    {
        PerformanceTimer perfTimer("queryOctree");
        quint64 sinceLastQuery = now - _lastQueriedTime;
        const quint64 TOO_LONG_SINCE_LAST_QUERY = 3 * USECS_PER_SECOND;
        bool queryIsDue = sinceLastQuery > TOO_LONG_SINCE_LAST_QUERY;
        bool viewIsDifferentEnough = !_lastQueriedViewFrustum.isVerySimilar(_viewFrustum);

        // if it's been a while since our last query or the view has significantly changed then send a query, otherwise suppress it
        if (queryIsDue || viewIsDifferentEnough) {
            _lastQueriedTime = now;

            if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
                queryOctree(NodeType::EntityServer, PacketTypeEntityQuery, _entityServerJurisdictions);
            }
            _lastQueriedViewFrustum = _viewFrustum;
        }
    }

    // sent nack packets containing missing sequence numbers of received packets from nodes
    {
        quint64 sinceLastNack = now - _lastNackTime;
        const quint64 TOO_LONG_SINCE_LAST_NACK = 1 * USECS_PER_SECOND;
        if (sinceLastNack > TOO_LONG_SINCE_LAST_NACK) {
            _lastNackTime = now;
            sendNackPackets();
        }
    }

    // send packet containing downstream audio stats to the AudioMixer
    {
        quint64 sinceLastNack = now - _lastSendDownstreamAudioStats;
        if (sinceLastNack > TOO_LONG_SINCE_LAST_SEND_DOWNSTREAM_AUDIO_STATS) {
            _lastSendDownstreamAudioStats = now;

            QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "sendDownstreamAudioStatsPacket", Qt::QueuedConnection);
        }
    }
}

int Application::sendNackPackets() {

    if (Menu::getInstance()->isOptionChecked(MenuOption::DisableNackPackets)) {
        return 0;
    }

    int packetsSent = 0;
    char packet[MAX_PACKET_SIZE];

    // iterates thru all nodes in NodeList
    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachNode([&](const SharedNodePointer& node){

        if (node->getActiveSocket() && node->getType() == NodeType::EntityServer) {

            QUuid nodeUUID = node->getUUID();

            // if there are octree packets from this node that are waiting to be processed,
            // don't send a NACK since the missing packets may be among those waiting packets.
            if (_octreeProcessor.hasPacketsToProcessFrom(nodeUUID)) {
                return;
            }

            _octreeSceneStatsLock.lockForRead();

            // retreive octree scene stats of this node
            if (_octreeServerSceneStats.find(nodeUUID) == _octreeServerSceneStats.end()) {
                _octreeSceneStatsLock.unlock();
                return;
            }

            // get sequence number stats of node, prune its missing set, and make a copy of the missing set
            SequenceNumberStats& sequenceNumberStats = _octreeServerSceneStats[nodeUUID].getIncomingOctreeSequenceNumberStats();
            sequenceNumberStats.pruneMissingSet();
            const QSet<OCTREE_PACKET_SEQUENCE> missingSequenceNumbers = sequenceNumberStats.getMissingSet();

            _octreeSceneStatsLock.unlock();

            // construct nack packet(s) for this node
            int numSequenceNumbersAvailable = missingSequenceNumbers.size();
            QSet<OCTREE_PACKET_SEQUENCE>::const_iterator missingSequenceNumbersIterator = missingSequenceNumbers.constBegin();
            while (numSequenceNumbersAvailable > 0) {

                char* dataAt = packet;
                int bytesRemaining = MAX_PACKET_SIZE;

                // pack header
                int numBytesPacketHeader = nodeList->populatePacketHeader(packet, PacketTypeOctreeDataNack);
                dataAt += numBytesPacketHeader;
                bytesRemaining -= numBytesPacketHeader;

                // calculate and pack the number of sequence numbers
                int numSequenceNumbersRoomFor = (bytesRemaining - sizeof(uint16_t)) / sizeof(OCTREE_PACKET_SEQUENCE);
                uint16_t numSequenceNumbers = min(numSequenceNumbersAvailable, numSequenceNumbersRoomFor);
                uint16_t* numSequenceNumbersAt = (uint16_t*)dataAt;
                *numSequenceNumbersAt = numSequenceNumbers;
                dataAt += sizeof(uint16_t);

                // pack sequence numbers
                for (int i = 0; i < numSequenceNumbers; i++) {
                    OCTREE_PACKET_SEQUENCE* sequenceNumberAt = (OCTREE_PACKET_SEQUENCE*)dataAt;
                    *sequenceNumberAt = *missingSequenceNumbersIterator;
                    dataAt += sizeof(OCTREE_PACKET_SEQUENCE);

                    missingSequenceNumbersIterator++;
                }
                numSequenceNumbersAvailable -= numSequenceNumbers;

                // send it
                nodeList->writeUnverifiedDatagram(packet, dataAt - packet, node);
                packetsSent++;
            }
        }
    });

    return packetsSent;
}

void Application::queryOctree(NodeType_t serverType, PacketType packetType, NodeToJurisdictionMap& jurisdictions) {

    //qCDebug(interfaceapp) << ">>> inside... queryOctree()... _viewFrustum.getFieldOfView()=" << _viewFrustum.getFieldOfView();
    bool wantExtraDebugging = getLogger()->extraDebugging();

    // These will be the same for all servers, so we can set them up once and then reuse for each server we send to.
    _octreeQuery.setWantLowResMoving(true);
    _octreeQuery.setWantColor(true);
    _octreeQuery.setWantDelta(true);
    _octreeQuery.setWantOcclusionCulling(false);
    _octreeQuery.setWantCompression(true);

    _octreeQuery.setCameraPosition(_viewFrustum.getPosition());
    _octreeQuery.setCameraOrientation(_viewFrustum.getOrientation());
    _octreeQuery.setCameraFov(_viewFrustum.getFieldOfView());
    _octreeQuery.setCameraAspectRatio(_viewFrustum.getAspectRatio());
    _octreeQuery.setCameraNearClip(_viewFrustum.getNearClip());
    _octreeQuery.setCameraFarClip(_viewFrustum.getFarClip());
    _octreeQuery.setCameraEyeOffsetPosition(glm::vec3());
    auto lodManager = DependencyManager::get<LODManager>();
    _octreeQuery.setOctreeSizeScale(lodManager->getOctreeSizeScale());
    _octreeQuery.setBoundaryLevelAdjust(lodManager->getBoundaryLevelAdjust());

    unsigned char queryPacket[MAX_PACKET_SIZE];

    // Iterate all of the nodes, and get a count of how many voxel servers we have...
    int totalServers = 0;
    int inViewServers = 0;
    int unknownJurisdictionServers = 0;

    auto nodeList = DependencyManager::get<NodeList>();

    nodeList->eachNode([&](const SharedNodePointer& node) {
        // only send to the NodeTypes that are serverType
        if (node->getActiveSocket() && node->getType() == serverType) {
            totalServers++;

            // get the server bounds for this server
            QUuid nodeUUID = node->getUUID();

            // if we haven't heard from this voxel server, go ahead and send it a query, so we
            // can get the jurisdiction...
            if (jurisdictions.find(nodeUUID) == jurisdictions.end()) {
                unknownJurisdictionServers++;
            } else {
                const JurisdictionMap& map = (jurisdictions)[nodeUUID];

                unsigned char* rootCode = map.getRootOctalCode();

                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    AACube serverBounds(glm::vec3(rootDetails.x * TREE_SCALE,
                                                  rootDetails.y * TREE_SCALE,
                                                  rootDetails.z * TREE_SCALE),
                                        rootDetails.s * TREE_SCALE);

                    ViewFrustum::location serverFrustumLocation = _viewFrustum.cubeInFrustum(serverBounds);

                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inViewServers++;
                    }
                }
            }
        }
    });

    if (wantExtraDebugging) {
        qCDebug(interfaceapp, "Servers: total %d, in view %d, unknown jurisdiction %d",
            totalServers, inViewServers, unknownJurisdictionServers);
    }

    int perServerPPS = 0;
    const int SMALL_BUDGET = 10;
    int perUnknownServer = SMALL_BUDGET;
    int totalPPS = getMaxOctreePacketsPerSecond();

    // determine PPS based on number of servers
    if (inViewServers >= 1) {
        // set our preferred PPS to be exactly evenly divided among all of the voxel servers... and allocate 1 PPS
        // for each unknown jurisdiction server
        perServerPPS = (totalPPS / inViewServers) - (unknownJurisdictionServers * perUnknownServer);
    } else {
        if (unknownJurisdictionServers > 0) {
            perUnknownServer = (totalPPS / unknownJurisdictionServers);
        }
    }

    if (wantExtraDebugging) {
        qCDebug(interfaceapp, "perServerPPS: %d perUnknownServer: %d", perServerPPS, perUnknownServer);
    }

    nodeList->eachNode([&](const SharedNodePointer& node){
        // only send to the NodeTypes that are serverType
        if (node->getActiveSocket() && node->getType() == serverType) {

            // get the server bounds for this server
            QUuid nodeUUID = node->getUUID();

            bool inView = false;
            bool unknownView = false;

            // if we haven't heard from this voxel server, go ahead and send it a query, so we
            // can get the jurisdiction...
            if (jurisdictions.find(nodeUUID) == jurisdictions.end()) {
                unknownView = true; // assume it's in view
                if (wantExtraDebugging) {
                    qCDebug(interfaceapp) << "no known jurisdiction for node " << *node << ", assume it's visible.";
                }
            } else {
                const JurisdictionMap& map = (jurisdictions)[nodeUUID];

                unsigned char* rootCode = map.getRootOctalCode();

                if (rootCode) {
                    VoxelPositionSize rootDetails;
                    voxelDetailsForCode(rootCode, rootDetails);
                    AACube serverBounds(glm::vec3(rootDetails.x * TREE_SCALE,
                                                  rootDetails.y * TREE_SCALE,
                                                  rootDetails.z * TREE_SCALE),
                                        rootDetails.s * TREE_SCALE);



                    ViewFrustum::location serverFrustumLocation = _viewFrustum.cubeInFrustum(serverBounds);
                    if (serverFrustumLocation != ViewFrustum::OUTSIDE) {
                        inView = true;
                    } else {
                        inView = false;
                    }
                } else {
                    if (wantExtraDebugging) {
                        qCDebug(interfaceapp) << "Jurisdiction without RootCode for node " << *node << ". That's unusual!";
                    }
                }
            }

            if (inView) {
                _octreeQuery.setMaxQueryPacketsPerSecond(perServerPPS);
            } else if (unknownView) {
                if (wantExtraDebugging) {
                    qCDebug(interfaceapp) << "no known jurisdiction for node " << *node << ", give it budget of "
                    << perUnknownServer << " to send us jurisdiction.";
                }

                // set the query's position/orientation to be degenerate in a manner that will get the scene quickly
                // If there's only one server, then don't do this, and just let the normal voxel query pass through
                // as expected... this way, we will actually get a valid scene if there is one to be seen
                if (totalServers > 1) {
                    _octreeQuery.setCameraPosition(glm::vec3(-0.1,-0.1,-0.1));
                    const glm::quat OFF_IN_NEGATIVE_SPACE = glm::quat(-0.5, 0, -0.5, 1.0);
                    _octreeQuery.setCameraOrientation(OFF_IN_NEGATIVE_SPACE);
                    _octreeQuery.setCameraNearClip(0.1f);
                    _octreeQuery.setCameraFarClip(0.1f);
                    if (wantExtraDebugging) {
                        qCDebug(interfaceapp) << "Using 'minimal' camera position for node" << *node;
                    }
                } else {
                    if (wantExtraDebugging) {
                        qCDebug(interfaceapp) << "Using regular camera position for node" << *node;
                    }
                }
                _octreeQuery.setMaxQueryPacketsPerSecond(perUnknownServer);
            } else {
                _octreeQuery.setMaxQueryPacketsPerSecond(0);
            }
            // set up the packet for sending...
            unsigned char* endOfQueryPacket = queryPacket;

            // insert packet type/version and node UUID
            endOfQueryPacket += nodeList->populatePacketHeader(reinterpret_cast<char*>(endOfQueryPacket), packetType);

            // encode the query data...
            endOfQueryPacket += _octreeQuery.getBroadcastData(endOfQueryPacket);

            int packetLength = endOfQueryPacket - queryPacket;

            // make sure we still have an active socket
            nodeList->writeUnverifiedDatagram(reinterpret_cast<const char*>(queryPacket), packetLength, node);
        }
    });
}

bool Application::isHMDMode() const {
    if (OculusManager::isConnected()) {
        return true;
    } else {
        return false;
    }
}

QRect Application::getDesirableApplicationGeometry() {
    QRect applicationGeometry = getWindow()->geometry();

    // If our parent window is on the HMD, then don't use its geometry, instead use
    // the "main screen" geometry.
    HMDToolsDialog* hmdTools = DependencyManager::get<DialogsManager>()->getHMDToolsDialog();
    if (hmdTools && hmdTools->hasHMDScreen()) {
        QScreen* hmdScreen = hmdTools->getHMDScreen();
        QWindow* appWindow = getWindow()->windowHandle();
        QScreen* appScreen = appWindow->screen();

        // if our app's screen is the hmd screen, we don't want to place the
        // running scripts widget on it. So we need to pick a better screen.
        // we will use the screen for the HMDTools since it's a guarenteed
        // better screen.
        if (appScreen == hmdScreen) {
            QScreen* betterScreen = hmdTools->windowHandle()->screen();
            applicationGeometry = betterScreen->geometry();
        }
    }
    return applicationGeometry;
}

/////////////////////////////////////////////////////////////////////////////////////
// loadViewFrustum()
//
// Description: this will load the view frustum bounds for EITHER the head
//                 or the "myCamera".
//
void Application::loadViewFrustum(Camera& camera, ViewFrustum& viewFrustum) {
    // We will use these below, from either the camera or head vectors calculated above
    viewFrustum.setProjection(camera.getProjection());

    // Set the viewFrustum up with the correct position and orientation of the camera
    viewFrustum.setPosition(camera.getPosition());
    viewFrustum.setOrientation(camera.getRotation());

    // Ask the ViewFrustum class to calculate our corners
    viewFrustum.calculate();
}

glm::vec3 Application::getSunDirection() {
    // Sun direction is in fact just the location of the sun relative to the origin
    auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
    return skyStage->getSunLight()->getDirection();
}

// FIXME, preprocessor guard this check to occur only in DEBUG builds
static QThread * activeRenderingThread = nullptr;

void Application::updateShadowMap(RenderArgs* renderArgs) {
    activeRenderingThread = QThread::currentThread();

    PerformanceTimer perfTimer("shadowMap");
    auto shadowFramebuffer = DependencyManager::get<TextureCache>()->getShadowFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(shadowFramebuffer));

    glEnable(GL_DEPTH_TEST);
    glClear(GL_DEPTH_BUFFER_BIT);

    glm::vec3 lightDirection = getSunDirection();
    glm::quat rotation = rotationBetween(IDENTITY_FRONT, lightDirection);
    glm::quat inverseRotation = glm::inverse(rotation);

    const float SHADOW_MATRIX_DISTANCES[] = { 0.0f, 2.0f, 6.0f, 14.0f, 30.0f };
    const glm::vec2 MAP_COORDS[] = { glm::vec2(0.0f, 0.0f), glm::vec2(0.5f, 0.0f),
        glm::vec2(0.0f, 0.5f), glm::vec2(0.5f, 0.5f) };

    float frustumScale = 1.0f / (_viewFrustum.getFarClip() - _viewFrustum.getNearClip());
    loadViewFrustum(_myCamera, _viewFrustum);

    int matrixCount = 1;
    //int targetSize = fbo->width();
    int sourceSize = shadowFramebuffer->getWidth();
    int targetSize = shadowFramebuffer->getWidth();
    float targetScale = 1.0f;
    if (Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows)) {
        matrixCount = CASCADED_SHADOW_MATRIX_COUNT;
        targetSize = sourceSize / 2;
        targetScale = 0.5f;
    }
    for (int i = 0; i < matrixCount; i++) {
        const glm::vec2& coord = MAP_COORDS[i];
        glViewport(coord.s * sourceSize, coord.t * sourceSize, targetSize, targetSize);

        // if simple shadow then since the resolution is twice as much as with cascaded, cover 2 regions with the map, not just one
        int regionIncrement = (matrixCount == 1 ? 2 : 1);
        float nearScale = SHADOW_MATRIX_DISTANCES[i] * frustumScale;
        float farScale = SHADOW_MATRIX_DISTANCES[i + regionIncrement] * frustumScale;
        glm::vec3 points[] = {
            glm::mix(_viewFrustum.getNearTopLeft(), _viewFrustum.getFarTopLeft(), nearScale),
            glm::mix(_viewFrustum.getNearTopRight(), _viewFrustum.getFarTopRight(), nearScale),
            glm::mix(_viewFrustum.getNearBottomLeft(), _viewFrustum.getFarBottomLeft(), nearScale),
            glm::mix(_viewFrustum.getNearBottomRight(), _viewFrustum.getFarBottomRight(), nearScale),
            glm::mix(_viewFrustum.getNearTopLeft(), _viewFrustum.getFarTopLeft(), farScale),
            glm::mix(_viewFrustum.getNearTopRight(), _viewFrustum.getFarTopRight(), farScale),
            glm::mix(_viewFrustum.getNearBottomLeft(), _viewFrustum.getFarBottomLeft(), farScale),
            glm::mix(_viewFrustum.getNearBottomRight(), _viewFrustum.getFarBottomRight(), farScale) };
        glm::vec3 center;
        for (size_t j = 0; j < sizeof(points) / sizeof(points[0]); j++) {
            center += points[j];
        }
        center /= (float)(sizeof(points) / sizeof(points[0]));
        float radius = 0.0f;
        for (size_t j = 0; j < sizeof(points) / sizeof(points[0]); j++) {
            radius = qMax(radius, glm::distance(points[j], center));
        }
        if (i < 3) {
            const float RADIUS_SCALE = 0.5f;
            _shadowDistances[i] = -glm::distance(_viewFrustum.getPosition(), center) - radius * RADIUS_SCALE;
        }
        center = inverseRotation * center;

        // to reduce texture "shimmer," move in texel increments
        float texelSize = (2.0f * radius) / targetSize;
        center = glm::vec3(roundf(center.x / texelSize) * texelSize, roundf(center.y / texelSize) * texelSize,
            roundf(center.z / texelSize) * texelSize);

        glm::vec3 minima(center.x - radius, center.y - radius, center.z - radius);
        glm::vec3 maxima(center.x + radius, center.y + radius, center.z + radius);

        // stretch out our extents in z so that we get all of the avatars
        minima.z -= _viewFrustum.getFarClip() * 0.5f;
        maxima.z += _viewFrustum.getFarClip() * 0.5f;

        // save the combined matrix for rendering
        _shadowMatrices[i] = glm::transpose(glm::translate(glm::vec3(coord, 0.0f)) *
            glm::scale(glm::vec3(targetScale, targetScale, 1.0f)) *
            glm::translate(glm::vec3(0.5f, 0.5f, 0.5f)) * glm::scale(glm::vec3(0.5f, 0.5f, 0.5f)) *
            glm::ortho(minima.x, maxima.x, minima.y, maxima.y, -maxima.z, -minima.z) * glm::mat4_cast(inverseRotation));

        // update the shadow view frustum
      //  glm::vec3 shadowFrustumCenter = glm::vec3((minima.x + maxima.x) * 0.5f, (minima.y + maxima.y) * 0.5f, (minima.z + maxima.z) * 0.5f);
        glm::vec3 shadowFrustumCenter = rotation * ((minima + maxima) * 0.5f);
        _shadowViewFrustum.setPosition(shadowFrustumCenter);
        _shadowViewFrustum.setOrientation(rotation);
        _shadowViewFrustum.setProjection(glm::ortho(minima.x, maxima.x, minima.y, maxima.y, minima.z, maxima.z));
        _shadowViewFrustum.calculate();

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(minima.x, maxima.x, minima.y, maxima.y, -maxima.z, -minima.z);

        glm::mat4 projAgain;
        glGetFloatv(GL_PROJECTION_MATRIX, (GLfloat*)&projAgain);


        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glm::vec3 axis = glm::axis(inverseRotation);
        glRotatef(glm::degrees(glm::angle(inverseRotation)), axis.x, axis.y, axis.z);

        // store view matrix without translation, which we'll use for precision-sensitive objects
        updateUntranslatedViewMatrix();

        // Equivalent to what is happening with _untranslatedViewMatrix and the _viewMatrixTranslation
        // the viewTransofmr object is updatded with the correct values and saved,
        // this is what is used for rendering the Entities and avatars
        Transform viewTransform;
        viewTransform.setRotation(rotation);
    //    viewTransform.postTranslate(shadowFrustumCenter);
        setViewTransform(viewTransform);


        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(1.1f, 4.0f); // magic numbers courtesy http://www.eecs.berkeley.edu/~ravir/6160/papers/shadowmaps.ppt

        {
            PerformanceTimer perfTimer("entities");
            _entities.render(renderArgs);
        }

        glDisable(GL_POLYGON_OFFSET_FILL);

        glPopMatrix();

        glMatrixMode(GL_PROJECTION);
        glPopMatrix();

        glMatrixMode(GL_MODELVIEW);
    }

   // fbo->release();

    glViewport(0, 0, _glWidget->getDeviceWidth(), _glWidget->getDeviceHeight());
    activeRenderingThread = nullptr;
}

const GLfloat WORLD_AMBIENT_COLOR[] = { 0.525f, 0.525f, 0.6f };
const GLfloat WORLD_DIFFUSE_COLOR[] = { 0.6f, 0.525f, 0.525f };
const GLfloat WORLD_SPECULAR_COLOR[] = { 0.08f, 0.08f, 0.08f, 1.0f };

const glm::vec3 GLOBAL_LIGHT_COLOR = {  0.6f, 0.525f, 0.525f };

void Application::setupWorldLight() {

    //  Setup 3D lights (after the camera transform, so that they are positioned in world space)
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glm::vec3 sunDirection = getSunDirection();
    GLfloat light_position0[] = { sunDirection.x, sunDirection.y, sunDirection.z, 0.0 };
    glLightfv(GL_LIGHT0, GL_POSITION, light_position0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, WORLD_AMBIENT_COLOR);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, WORLD_DIFFUSE_COLOR);
    glLightfv(GL_LIGHT0, GL_SPECULAR, WORLD_SPECULAR_COLOR);
    glMaterialfv(GL_FRONT, GL_SPECULAR, WORLD_SPECULAR_COLOR);
    glMateriali(GL_FRONT, GL_SHININESS, 96);

}

bool Application::shouldRenderMesh(float largestDimension, float distanceToCamera) {
    return DependencyManager::get<LODManager>()->shouldRenderMesh(largestDimension, distanceToCamera);
}

float Application::getSizeScale() const {
    return DependencyManager::get<LODManager>()->getOctreeSizeScale();
}

int Application::getBoundaryLevelAdjust() const {
    return DependencyManager::get<LODManager>()->getBoundaryLevelAdjust();
}

PickRay Application::computePickRay(float x, float y) const {
    glm::vec2 size = getCanvasSize();
    x /= size.x;
    y /= size.y;
    PickRay result;
    if (isHMDMode()) {
        getApplicationCompositor().computeHmdPickRay(glm::vec2(x, y), result.origin, result.direction);
    } else {
        if (QThread::currentThread() == activeRenderingThread) {
            getDisplayViewFrustum()->computePickRay(x, y, result.origin, result.direction);
        } else {
            getViewFrustum()->computePickRay(x, y, result.origin, result.direction);
        }
    }
    return result;
}

QImage Application::renderAvatarBillboard(RenderArgs* renderArgs) {
    auto primaryFramebuffer = DependencyManager::get<TextureCache>()->getPrimaryFramebuffer();
    glBindFramebuffer(GL_FRAMEBUFFER, gpu::GLBackend::getFramebufferID(primaryFramebuffer));

    // clear the alpha channel so the background is transparent
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

    // the "glow" here causes an alpha of one
    Glower glower(renderArgs);

    const int BILLBOARD_SIZE = 64;
    // TODO: Pass a RenderArgs to renderAvatarBillboard
    renderRearViewMirror(renderArgs, QRect(0, _glWidget->getDeviceHeight() - BILLBOARD_SIZE,
                               BILLBOARD_SIZE, BILLBOARD_SIZE),
                         true);
    QImage image(BILLBOARD_SIZE, BILLBOARD_SIZE, QImage::Format_ARGB32);
    glReadPixels(0, 0, BILLBOARD_SIZE, BILLBOARD_SIZE, GL_BGRA, GL_UNSIGNED_BYTE, image.bits());
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return image;
}

ViewFrustum* Application::getViewFrustum() {
#ifdef DEBUG
    if (QThread::currentThread() == activeRenderingThread) {
        // FIXME, figure out a better way to do this
        //qWarning() << "Calling Application::getViewFrustum() from the active rendering thread, did you mean Application::getDisplayViewFrustum()?";
    }
#endif
    return &_viewFrustum;
}

const ViewFrustum* Application::getViewFrustum() const {
#ifdef DEBUG
    if (QThread::currentThread() == activeRenderingThread) {
        // FIXME, figure out a better way to do this
        //qWarning() << "Calling Application::getViewFrustum() from the active rendering thread, did you mean Application::getDisplayViewFrustum()?";
    }
#endif
    return &_viewFrustum;
}

ViewFrustum* Application::getDisplayViewFrustum() {
#ifdef DEBUG
    if (QThread::currentThread() != activeRenderingThread) {
        // FIXME, figure out a better way to do this
        // qWarning() << "Calling Application::getDisplayViewFrustum() from outside the active rendering thread or outside rendering, did you mean Application::getViewFrustum()?";
    }
#endif
    return &_displayViewFrustum;
}

const ViewFrustum* Application::getDisplayViewFrustum() const {
#ifdef DEBUG
    if (QThread::currentThread() != activeRenderingThread) {
        // FIXME, figure out a better way to do this
        // qWarning() << "Calling Application::getDisplayViewFrustum() from outside the active rendering thread or outside rendering, did you mean Application::getViewFrustum()?";
    }
#endif
    return &_displayViewFrustum;
}

// WorldBox Render Data & rendering functions

class WorldBoxRenderData {
public:
    typedef render::Payload<WorldBoxRenderData> Payload;
    typedef Payload::DataPointer Pointer;

    int _val = 0;
    static render::ItemID _item; // unique WorldBoxRenderData
};

render::ItemID WorldBoxRenderData::_item = 0;

namespace render {
    template <> const ItemKey payloadGetKey(const WorldBoxRenderData::Pointer& stuff) { return ItemKey::Builder::opaqueShape(); }
    template <> const Item::Bound payloadGetBound(const WorldBoxRenderData::Pointer& stuff) { return Item::Bound(); }
    template <> void payloadRender(const WorldBoxRenderData::Pointer& stuff, RenderArgs* args) {
        if (args->_renderMode != RenderArgs::MIRROR_RENDER_MODE && Menu::getInstance()->isOptionChecked(MenuOption::Stats)) {
            PerformanceTimer perfTimer("worldBox");

            auto& batch = *args->_batch;
            DependencyManager::get<DeferredLightingEffect>()->bindSimpleProgram(batch);
            renderWorldBox(batch);
        }
    }
}

// Background Render Data & rendering functions
class BackgroundRenderData {
public:
    typedef render::Payload<BackgroundRenderData> Payload;
    typedef Payload::DataPointer Pointer;

    Stars _stars;
    Environment* _environment;

    BackgroundRenderData(Environment* environment) : _environment(environment) {
    }

    static render::ItemID _item; // unique WorldBoxRenderData
};

render::ItemID BackgroundRenderData::_item = 0;

namespace render {
    template <> const ItemKey payloadGetKey(const BackgroundRenderData::Pointer& stuff) { return ItemKey::Builder::background(); }
    template <> const Item::Bound payloadGetBound(const BackgroundRenderData::Pointer& stuff) { return Item::Bound(); }
    template <> void payloadRender(const BackgroundRenderData::Pointer& background, RenderArgs* args) {

        Q_ASSERT(args->_batch);
        gpu::Batch& batch = *args->_batch;

        // Background rendering decision
        auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
        auto skybox = model::SkyboxPointer();
        if (skyStage->getBackgroundMode() == model::SunSkyStage::NO_BACKGROUND) {
        } else if (skyStage->getBackgroundMode() == model::SunSkyStage::SKY_DOME) {
           if (/*!selfAvatarOnly &&*/ Menu::getInstance()->isOptionChecked(MenuOption::Stars)) {
                PerformanceTimer perfTimer("stars");
                PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                    "Application::payloadRender<BackgroundRenderData>() ... stars...");
                if (!background->_stars.isStarsLoaded()) {
                    background->_stars.generate(STARFIELD_NUM_STARS, STARFIELD_SEED);
                }
                // should be the first rendering pass - w/o depth buffer / lighting

                // compute starfield alpha based on distance from atmosphere
                float alpha = 1.0f;
                bool hasStars = true;

                if (Menu::getInstance()->isOptionChecked(MenuOption::Atmosphere)) {
                    // TODO: handle this correctly for zones
                    const EnvironmentData& closestData = background->_environment->getClosestData(args->_viewFrustum->getPosition()); // was theCamera instead of  _viewFrustum

                    if (closestData.getHasStars()) {
                        const float APPROXIMATE_DISTANCE_FROM_HORIZON = 0.1f;
                        const float DOUBLE_APPROXIMATE_DISTANCE_FROM_HORIZON = 0.2f;

                        glm::vec3 sunDirection = (args->_viewFrustum->getPosition()/*getAvatarPosition()*/ - closestData.getSunLocation()) 
                                                        / closestData.getAtmosphereOuterRadius();
                        float height = glm::distance(args->_viewFrustum->getPosition()/*theCamera.getPosition()*/, closestData.getAtmosphereCenter());
                        if (height < closestData.getAtmosphereInnerRadius()) {
                            // If we're inside the atmosphere, then determine if our keyLight is below the horizon
                            alpha = 0.0f;

                            if (sunDirection.y > -APPROXIMATE_DISTANCE_FROM_HORIZON) {
                                float directionY = glm::clamp(sunDirection.y, 
                                                    -APPROXIMATE_DISTANCE_FROM_HORIZON, APPROXIMATE_DISTANCE_FROM_HORIZON) 
                                                    + APPROXIMATE_DISTANCE_FROM_HORIZON;
                                alpha = (directionY / DOUBLE_APPROXIMATE_DISTANCE_FROM_HORIZON);
                            }
                        

                        } else if (height < closestData.getAtmosphereOuterRadius()) {
                            alpha = (height - closestData.getAtmosphereInnerRadius()) /
                                (closestData.getAtmosphereOuterRadius() - closestData.getAtmosphereInnerRadius());

                            if (sunDirection.y > -APPROXIMATE_DISTANCE_FROM_HORIZON) {
                                float directionY = glm::clamp(sunDirection.y, 
                                                    -APPROXIMATE_DISTANCE_FROM_HORIZON, APPROXIMATE_DISTANCE_FROM_HORIZON) 
                                                    + APPROXIMATE_DISTANCE_FROM_HORIZON;
                                alpha = (directionY / DOUBLE_APPROXIMATE_DISTANCE_FROM_HORIZON);
                            }
                        }
                    } else {
                        hasStars = false;
                    }
                }

                // finally render the starfield
                if (hasStars) {
                    background->_stars.render(args->_viewFrustum->getFieldOfView(), args->_viewFrustum->getAspectRatio(), args->_viewFrustum->getNearClip(), alpha);
                }

                // draw the sky dome
                if (/*!selfAvatarOnly &&*/ Menu::getInstance()->isOptionChecked(MenuOption::Atmosphere)) {
                    PerformanceTimer perfTimer("atmosphere");
                    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                        "Application::displaySide() ... atmosphere...");

                    background->_environment->renderAtmospheres(batch, *(args->_viewFrustum));
                }

            }
        } else if (skyStage->getBackgroundMode() == model::SunSkyStage::SKY_BOX) {
            PerformanceTimer perfTimer("skybox");
            
            skybox = skyStage->getSkybox();
            if (skybox) {
                model::Skybox::render(batch, *(Application::getInstance()->getDisplayViewFrustum()), *skybox);
            }
        }
        // FIX ME - If I don't call this renderBatch() here, then the atmosphere and skybox don't render, but it
        // seems like these payloadRender() methods shouldn't be doing this. We need to investigate why the engine
        // isn't rendering our batch
        gpu::GLBackend::renderBatch(batch, true);
    }
}


void Application::displaySide(RenderArgs* renderArgs, Camera& theCamera, bool selfAvatarOnly, bool billboard) {

    // FIXME: This preRender call is temporary until we create a separate render::scene for the mirror rendering.
    // Then we can move this logic into the Avatar::simulate call.
    _myAvatar->preRender(renderArgs);

    activeRenderingThread = QThread::currentThread();
    PROFILE_RANGE(__FUNCTION__);
    PerformanceTimer perfTimer("display");
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings), "Application::displaySide()");
    // transform by eye offset

    // load the view frustum
    loadViewFrustum(theCamera, _displayViewFrustum);

    // flip x if in mirror mode (also requires reversing winding order for backface culling)
    if (theCamera.getMode() == CAMERA_MODE_MIRROR) {
        //glScalef(-1.0f, 1.0f, 1.0f);
        //glFrontFace(GL_CW);
    } else {
        glFrontFace(GL_CCW);
    }

    // transform view according to theCamera
    // could be myCamera (if in normal mode)
    // or could be viewFrustumOffsetCamera if in offset mode

    glm::quat rotation = theCamera.getRotation();
    glm::vec3 axis = glm::axis(rotation);
    glRotatef(-glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);

    // store view matrix without translation, which we'll use for precision-sensitive objects
    updateUntranslatedViewMatrix(-theCamera.getPosition());

    // Equivalent to what is happening with _untranslatedViewMatrix and the _viewMatrixTranslation
    // the viewTransofmr object is updatded with the correct values and saved,
    // this is what is used for rendering the Entities and avatars
    Transform viewTransform;
    viewTransform.setTranslation(theCamera.getPosition());
    viewTransform.setRotation(rotation);
    if (theCamera.getMode() == CAMERA_MODE_MIRROR) {
//         viewTransform.setScale(Transform::Vec3(-1.0f, 1.0f, 1.0f));
    }
    if (renderArgs->_renderSide != RenderArgs::MONO) {
        glm::mat4 invView = glm::inverse(_untranslatedViewMatrix);

        viewTransform.evalFromRawMatrix(invView);
        viewTransform.preTranslate(_viewMatrixTranslation);
    }

    setViewTransform(viewTransform);

    glTranslatef(_viewMatrixTranslation.x, _viewMatrixTranslation.y, _viewMatrixTranslation.z);

    //  Setup 3D lights (after the camera transform, so that they are positioned in world space)
    {
        PerformanceTimer perfTimer("lights");
        setupWorldLight();
    }

    // setup shadow matrices (again, after the camera transform)
    int shadowMatrixCount = 0;
    if (Menu::getInstance()->isOptionChecked(MenuOption::SimpleShadows)) {
        shadowMatrixCount = 1;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows)) {
        shadowMatrixCount = CASCADED_SHADOW_MATRIX_COUNT;
    }
    for (int i = shadowMatrixCount - 1; i >= 0; i--) {
        glActiveTexture(GL_TEXTURE0 + i);
        glTexGenfv(GL_S, GL_EYE_PLANE, (const GLfloat*)&_shadowMatrices[i][0]);
        glTexGenfv(GL_T, GL_EYE_PLANE, (const GLfloat*)&_shadowMatrices[i][1]);
        glTexGenfv(GL_R, GL_EYE_PLANE, (const GLfloat*)&_shadowMatrices[i][2]);
    }

    // The pending changes collecting the changes here
    render::PendingChanges pendingChanges;

    // Background rendering decision
    if (BackgroundRenderData::_item == 0) {
        auto backgroundRenderData = BackgroundRenderData::Pointer(new BackgroundRenderData(&_environment));
        auto backgroundRenderPayload = render::PayloadPointer(new BackgroundRenderData::Payload(backgroundRenderData));
        BackgroundRenderData::_item = _main3DScene->allocateID();
        pendingChanges.resetItem(WorldBoxRenderData::_item, backgroundRenderPayload);
    } else {

    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::Wireframe)) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    
   // Assuming nothing get's rendered through that

    if (!selfAvatarOnly) {

        // render models...
        if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderEntities()) {
            PerformanceTimer perfTimer("entities");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... entities...");

            RenderArgs::DebugFlags renderDebugFlags = RenderArgs::RENDER_DEBUG_NONE;

            if (Menu::getInstance()->isOptionChecked(MenuOption::PhysicsShowHulls)) {
                renderDebugFlags = (RenderArgs::DebugFlags) (renderDebugFlags | (int) RenderArgs::RENDER_DEBUG_HULLS);
            }
            if (Menu::getInstance()->isOptionChecked(MenuOption::PhysicsShowOwned)) {
                renderDebugFlags =
                    (RenderArgs::DebugFlags) (renderDebugFlags | (int) RenderArgs::RENDER_DEBUG_SIMULATION_OWNERSHIP);
            }
            renderArgs->_debugFlags = renderDebugFlags;
            _entities.render(renderArgs);
        }

        // render the ambient occlusion effect if enabled
        if (Menu::getInstance()->isOptionChecked(MenuOption::AmbientOcclusion)) {
            PerformanceTimer perfTimer("ambientOcclusion");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... AmbientOcclusion...");
            DependencyManager::get<AmbientOcclusionEffect>()->render();
        }
    }

    // Make sure the WorldBox is in the scene
    if (WorldBoxRenderData::_item == 0) {
        auto worldBoxRenderData = WorldBoxRenderData::Pointer(new WorldBoxRenderData());
        auto worldBoxRenderPayload = render::PayloadPointer(new WorldBoxRenderData::Payload(worldBoxRenderData));

        WorldBoxRenderData::_item = _main3DScene->allocateID();

        pendingChanges.resetItem(WorldBoxRenderData::_item, worldBoxRenderPayload);
    } else {

        pendingChanges.updateItem<WorldBoxRenderData>(WorldBoxRenderData::_item,  
                [](WorldBoxRenderData& payload) { 
                    payload._val++;
                });
    }

    if (!billboard) {
        DependencyManager::get<DeferredLightingEffect>()->setAmbientLightMode(getRenderAmbientLight());
        auto skyStage = DependencyManager::get<SceneScriptingInterface>()->getSkyStage();
        DependencyManager::get<DeferredLightingEffect>()->setGlobalLight(skyStage->getSunLight()->getDirection(), skyStage->getSunLight()->getColor(), skyStage->getSunLight()->getIntensity(), skyStage->getSunLight()->getAmbientIntensity());
        DependencyManager::get<DeferredLightingEffect>()->setGlobalAtmosphere(skyStage->getAtmosphere());

        auto skybox = model::SkyboxPointer();
        if (skyStage->getBackgroundMode() == model::SunSkyStage::SKY_BOX) {
            skybox = skyStage->getSkybox();
        }
        DependencyManager::get<DeferredLightingEffect>()->setGlobalSkybox(skybox);
    }

    {
        PerformanceTimer perfTimer("SceneProcessPendingChanges"); 
        _main3DScene->enqueuePendingChanges(pendingChanges);

        _main3DScene->processPendingChangesQueue();
    }

    // For now every frame pass the renderContext
    {
        PerformanceTimer perfTimer("EngineRun");
        render::RenderContext renderContext;

        auto sceneInterface = DependencyManager::get<SceneScriptingInterface>();

        renderContext._cullOpaque = sceneInterface->doEngineCullOpaque();
        renderContext._sortOpaque = sceneInterface->doEngineSortOpaque();
        renderContext._renderOpaque = sceneInterface->doEngineRenderOpaque();
        renderContext._cullTransparent = sceneInterface->doEngineCullTransparent();
        renderContext._sortTransparent = sceneInterface->doEngineSortTransparent();
        renderContext._renderTransparent = sceneInterface->doEngineRenderTransparent();

        renderContext._maxDrawnOpaqueItems = sceneInterface->getEngineMaxDrawnOpaqueItems();
        renderContext._maxDrawnTransparentItems = sceneInterface->getEngineMaxDrawnTransparentItems();
        renderContext._maxDrawnOverlay3DItems = sceneInterface->getEngineMaxDrawnOverlay3DItems();

        renderContext._drawItemStatus = sceneInterface->doEngineDisplayItemStatus();

        renderArgs->_shouldRender = LODManager::shouldRender;

        renderContext.args = renderArgs;
        renderArgs->_viewFrustum = getDisplayViewFrustum();
        _renderEngine->setRenderContext(renderContext);

        // Before the deferred pass, let's try to use the render engine
        _renderEngine->run();
        
        auto engineRC = _renderEngine->getRenderContext();
        sceneInterface->setEngineFeedOpaqueItems(engineRC->_numFeedOpaqueItems);
        sceneInterface->setEngineDrawnOpaqueItems(engineRC->_numDrawnOpaqueItems);

        sceneInterface->setEngineFeedTransparentItems(engineRC->_numFeedTransparentItems);
        sceneInterface->setEngineDrawnTransparentItems(engineRC->_numDrawnTransparentItems);

        sceneInterface->setEngineFeedOverlay3DItems(engineRC->_numFeedOverlay3DItems);
        sceneInterface->setEngineDrawnOverlay3DItems(engineRC->_numDrawnOverlay3DItems);
    }
    //Render the sixense lasers
    if (Menu::getInstance()->isOptionChecked(MenuOption::SixenseLasers)) {
        _myAvatar->renderLaserPointers(*renderArgs->_batch);
    }

    if (!selfAvatarOnly) {
        _nodeBoundsDisplay.draw();

        // render octree fades if they exist
        if (_octreeFades.size() > 0) {
            PerformanceTimer perfTimer("octreeFades");
            PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                "Application::displaySide() ... octree fades...");
            _octreeFadesLock.lockForWrite();
            for(std::vector<OctreeFade>::iterator fade = _octreeFades.begin(); fade != _octreeFades.end();) {
                fade->render(renderArgs);
                if(fade->isDone()) {
                    fade = _octreeFades.erase(fade);
                } else {
                    ++fade;
                }
            }
            _octreeFadesLock.unlock();
        }

        // give external parties a change to hook in
        {
            PerformanceTimer perfTimer("inWorldInterface");
            emit renderingInWorldInterface();
        }
    }

    if (Menu::getInstance()->isOptionChecked(MenuOption::Wireframe)) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    activeRenderingThread = nullptr;
}

void Application::updateUntranslatedViewMatrix(const glm::vec3& viewMatrixTranslation) {
    glGetFloatv(GL_MODELVIEW_MATRIX, (GLfloat*)&_untranslatedViewMatrix);
    _viewMatrixTranslation = viewMatrixTranslation;
}

void Application::setViewTransform(const Transform& view) {
    _viewTransform = view;
}

void Application::loadTranslatedViewMatrix(const glm::vec3& translation) {
    glLoadMatrixf((const GLfloat*)&_untranslatedViewMatrix);
    glTranslatef(translation.x + _viewMatrixTranslation.x, translation.y + _viewMatrixTranslation.y,
        translation.z + _viewMatrixTranslation.z);
}

void Application::getModelViewMatrix(glm::dmat4* modelViewMatrix) {
    (*modelViewMatrix) =_untranslatedViewMatrix;
    (*modelViewMatrix)[3] = _untranslatedViewMatrix * glm::vec4(_viewMatrixTranslation, 1);
}

void Application::getProjectionMatrix(glm::dmat4* projectionMatrix) {
    *projectionMatrix = _projectionMatrix;
}

void Application::computeOffAxisFrustum(float& left, float& right, float& bottom, float& top, float& nearVal,
    float& farVal, glm::vec4& nearClipPlane, glm::vec4& farClipPlane) const {

    // allow 3DTV/Oculus to override parameters from camera
    _displayViewFrustum.computeOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    if (OculusManager::isConnected()) {
        OculusManager::overrideOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);

    } else if (TV3DManager::isConnected()) {
        TV3DManager::overrideOffAxisFrustum(left, right, bottom, top, nearVal, farVal, nearClipPlane, farClipPlane);
    }
}

bool Application::getShadowsEnabled() {
    Menu* menubar = Menu::getInstance();
    return menubar->isOptionChecked(MenuOption::SimpleShadows) ||
           menubar->isOptionChecked(MenuOption::CascadedShadows);
}

bool Application::getCascadeShadowsEnabled() {
    return Menu::getInstance()->isOptionChecked(MenuOption::CascadedShadows);
}

glm::vec2 Application::getScaledScreenPoint(glm::vec2 projectedPoint) {
    float horizontalScale = _glWidget->getDeviceWidth() / 2.0f;
    float verticalScale   = _glWidget->getDeviceHeight() / 2.0f;

    // -1,-1 is 0,windowHeight
    // 1,1 is windowWidth,0

    // -1,1                    1,1
    // +-----------------------+
    // |           |           |
    // |           |           |
    // | -1,0      |           |
    // |-----------+-----------|
    // |          0,0          |
    // |           |           |
    // |           |           |
    // |           |           |
    // +-----------------------+
    // -1,-1                   1,-1

    glm::vec2 screenPoint((projectedPoint.x + 1.0f) * horizontalScale,
        ((projectedPoint.y + 1.0f) * -verticalScale) + _glWidget->getDeviceHeight());

    return screenPoint;
}

void Application::renderRearViewMirror(RenderArgs* renderArgs, const QRect& region, bool billboard) {
    // Grab current viewport to reset it at the end
    int viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    float aspect = (float)region.width() / region.height();
    float fov = MIRROR_FIELD_OF_VIEW;

    // bool eyeRelativeCamera = false;
    if (billboard) {
        fov = BILLBOARD_FIELD_OF_VIEW;  // degees
        _mirrorCamera.setPosition(_myAvatar->getPosition() +
                                  _myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * BILLBOARD_DISTANCE * _myAvatar->getScale());

    } else if (!AvatarInputs::getInstance()->mirrorZoomed()) {
        _mirrorCamera.setPosition(_myAvatar->getChestPosition() +
                                  _myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_BODY_DISTANCE * _myAvatar->getScale());

    } else { // HEAD zoom level
        // FIXME note that the positioing of the camera relative to the avatar can suffer limited
        // precision as the user's position moves further away from the origin.  Thus at
        // /1e7,1e7,1e7 (well outside the buildable volume) the mirror camera veers and sways
        // wildly as you rotate your avatar because the floating point values are becoming
        // larger, squeezing out the available digits of precision you have available at the
        // human scale for camera positioning.

        // Previously there was a hack to correct this using the mechanism of repositioning
        // the avatar at the origin of the world for the purposes of rendering the mirror,
        // but it resulted in failing to render the avatar's head model in the mirror view
        // when in first person mode.  Presumably this was because of some missed culling logic
        // that was not accounted for in the hack.

        // This was removed in commit 71e59cfa88c6563749594e25494102fe01db38e9 but could be further
        // investigated in order to adapt the technique while fixing the head rendering issue,
        // but the complexity of the hack suggests that a better approach
        _mirrorCamera.setPosition(_myAvatar->getHead()->getEyePosition() +
                                    _myAvatar->getOrientation() * glm::vec3(0.0f, 0.0f, -1.0f) * MIRROR_REARVIEW_DISTANCE * _myAvatar->getScale());
    }
    _mirrorCamera.setProjection(glm::perspective(glm::radians(fov), aspect, DEFAULT_NEAR_CLIP, DEFAULT_FAR_CLIP));
    _mirrorCamera.setRotation(_myAvatar->getWorldAlignedOrientation() * glm::quat(glm::vec3(0.0f, PI, 0.0f)));

    // set the bounds of rear mirror view
    if (billboard) {
        QSize size = DependencyManager::get<TextureCache>()->getFrameBufferSize();
        glViewport(region.x(), size.height() - region.y() - region.height(), region.width(), region.height());
        glScissor(region.x(), size.height() - region.y() - region.height(), region.width(), region.height());
    } else {
        // if not rendering the billboard, the region is in device independent coordinates; must convert to device
        QSize size = DependencyManager::get<TextureCache>()->getFrameBufferSize();
        float ratio = (float)QApplication::desktop()->windowHandle()->devicePixelRatio() * getRenderResolutionScale();
        int x = region.x() * ratio, y = region.y() * ratio, width = region.width() * ratio, height = region.height() * ratio;
        glViewport(x, size.height() - y - height, width, height);
        glScissor(x, size.height() - y - height, width, height);
    }
    bool updateViewFrustum = false;
    updateProjectionMatrix(_mirrorCamera, updateViewFrustum);
    glEnable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render rear mirror view
    glPushMatrix();
    displaySide(renderArgs, _mirrorCamera, true, billboard);
    glPopMatrix();

    // reset Viewport and projection matrix
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    glDisable(GL_SCISSOR_TEST);
    updateProjectionMatrix(_myCamera, updateViewFrustum);
}

void Application::resetSensors() {
    DependencyManager::get<Faceshift>()->reset();
    DependencyManager::get<DdeFaceTracker>()->reset();

    OculusManager::reset();

    //_leapmotion.reset();

    QScreen* currentScreen = _window->windowHandle()->screen();
    QWindow* mainWindow = _window->windowHandle();
    QPoint windowCenter = mainWindow->geometry().center();
    _glWidget->cursor().setPos(currentScreen, windowCenter);

    _myAvatar->reset();

    QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "reset", Qt::QueuedConnection);
}

static void setShortcutsEnabled(QWidget* widget, bool enabled) {
    foreach (QAction* action, widget->actions()) {
        QKeySequence shortcut = action->shortcut();
        if (!shortcut.isEmpty() && (shortcut[0] & (Qt::CTRL | Qt::ALT | Qt::META)) == 0) {
            // it's a shortcut that may coincide with a "regular" key, so switch its context
            action->setShortcutContext(enabled ? Qt::WindowShortcut : Qt::WidgetShortcut);
        }
    }
    foreach (QObject* child, widget->children()) {
        if (child->isWidgetType()) {
            setShortcutsEnabled(static_cast<QWidget*>(child), enabled);
        }
    }
}

void Application::setMenuShortcutsEnabled(bool enabled) {
    setShortcutsEnabled(_window->menuBar(), enabled);
}

void Application::updateWindowTitle(){

    QString buildVersion = " (build " + applicationVersion() + ")";
    auto nodeList = DependencyManager::get<NodeList>();

    QString connectionStatus = nodeList->getDomainHandler().isConnected() ? "" : " (NOT CONNECTED) ";
    QString username = AccountManager::getInstance().getAccountInfo().getUsername();
    QString currentPlaceName = DependencyManager::get<AddressManager>()->getHost();

    if (currentPlaceName.isEmpty()) {
        currentPlaceName = nodeList->getDomainHandler().getHostname();
    }

    QString title = QString() + (!username.isEmpty() ? username + " @ " : QString())
        + currentPlaceName + connectionStatus + buildVersion;

#ifndef WIN32
    // crashes with vs2013/win32
    qCDebug(interfaceapp, "Application title set to: %s", title.toStdString().c_str());
#endif
    _window->setWindowTitle(title);
}

void Application::clearDomainOctreeDetails() {
    qCDebug(interfaceapp) << "Clearing domain octree details...";
    // reset the environment so that we don't erroneously end up with multiple

    // reset our node to stats and node to jurisdiction maps... since these must be changing...
    _entityServerJurisdictions.lockForWrite();
    _entityServerJurisdictions.clear();
    _entityServerJurisdictions.unlock();

    _octreeSceneStatsLock.lockForWrite();
    _octreeServerSceneStats.clear();
    _octreeSceneStatsLock.unlock();

    // reset the model renderer
    _entities.clear();

}

void Application::domainChanged(const QString& domainHostname) {
    updateWindowTitle();
    clearDomainOctreeDetails();
    _domainConnectionRefusals.clear();
}

void Application::domainConnectionDenied(const QString& reason) {
    if (!_domainConnectionRefusals.contains(reason)) {
        _domainConnectionRefusals.append(reason);
        emit domainConnectionRefused(reason);
    }
}

void Application::connectedToDomain(const QString& hostname) {
    AccountManager& accountManager = AccountManager::getInstance();
    const QUuid& domainID = DependencyManager::get<NodeList>()->getDomainHandler().getUUID();

    if (accountManager.isLoggedIn() && !domainID.isNull()) {
        _notifiedPacketVersionMismatchThisDomain = false;
    }
}

void Application::nodeAdded(SharedNodePointer node) {
    if (node->getType() == NodeType::AvatarMixer) {
        // new avatar mixer, send off our identity packet right away
        _myAvatar->sendIdentityPacket();
    }
}

void Application::nodeKilled(SharedNodePointer node) {

    // These are here because connecting NodeList::nodeKilled to OctreePacketProcessor::nodeKilled doesn't work:
    // OctreePacketProcessor::nodeKilled is not being called when NodeList::nodeKilled is emitted.
    // This may have to do with GenericThread::threadRoutine() blocking the QThread event loop

    _octreeProcessor.nodeKilled(node);

    _entityEditSender.nodeKilled(node);

    if (node->getType() == NodeType::AudioMixer) {
        QMetaObject::invokeMethod(DependencyManager::get<AudioClient>().data(), "audioMixerKilled");
    }

    if (node->getType() == NodeType::EntityServer) {

        QUuid nodeUUID = node->getUUID();
        // see if this is the first we've heard of this node...
        _entityServerJurisdictions.lockForRead();
        if (_entityServerJurisdictions.find(nodeUUID) != _entityServerJurisdictions.end()) {
            unsigned char* rootCode = _entityServerJurisdictions[nodeUUID].getRootOctalCode();
            VoxelPositionSize rootDetails;
            voxelDetailsForCode(rootCode, rootDetails);
            _entityServerJurisdictions.unlock();

            qCDebug(interfaceapp, "model server going away...... v[%f, %f, %f, %f]",
                    (double)rootDetails.x, (double)rootDetails.y, (double)rootDetails.z, (double)rootDetails.s);

            // Add the jurisditionDetails object to the list of "fade outs"
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontFadeOnOctreeServerChanges)) {
                OctreeFade fade(OctreeFade::FADE_OUT, NODE_KILLED_RED, NODE_KILLED_GREEN, NODE_KILLED_BLUE);
                fade.voxelDetails = rootDetails;
                const float slightly_smaller = 0.99f;
                fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
                _octreeFadesLock.lockForWrite();
                _octreeFades.push_back(fade);
                _octreeFadesLock.unlock();
            }

            // If the model server is going away, remove it from our jurisdiction map so we don't send voxels to a dead server
            _entityServerJurisdictions.lockForWrite();
            _entityServerJurisdictions.erase(_entityServerJurisdictions.find(nodeUUID));
        }
        _entityServerJurisdictions.unlock();

        // also clean up scene stats for that server
        _octreeSceneStatsLock.lockForWrite();
        if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
            _octreeServerSceneStats.erase(nodeUUID);
        }
        _octreeSceneStatsLock.unlock();

    } else if (node->getType() == NodeType::AvatarMixer) {
        // our avatar mixer has gone away - clear the hash of avatars
        DependencyManager::get<AvatarManager>()->clearOtherAvatars();
    }
}

void Application::trackIncomingOctreePacket(const QByteArray& packet, const SharedNodePointer& sendingNode, bool wasStatsPacket) {

    // Attempt to identify the sender from its address.
    if (sendingNode) {
        QUuid nodeUUID = sendingNode->getUUID();

        // now that we know the node ID, let's add these stats to the stats for that node...
        _octreeSceneStatsLock.lockForWrite();
        if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
            OctreeSceneStats& stats = _octreeServerSceneStats[nodeUUID];
            stats.trackIncomingOctreePacket(packet, wasStatsPacket, sendingNode->getClockSkewUsec());
        }
        _octreeSceneStatsLock.unlock();
    }
}

int Application::parseOctreeStats(const QByteArray& packet, const SharedNodePointer& sendingNode) {
    // But, also identify the sender, and keep track of the contained jurisdiction root for this server

    // parse the incoming stats datas stick it in a temporary object for now, while we
    // determine which server it belongs to
    OctreeSceneStats temp;
    int statsMessageLength = temp.unpackFromMessage(reinterpret_cast<const unsigned char*>(packet.data()), packet.size());

    // quick fix for crash... why would voxelServer be NULL?
    if (sendingNode) {
        QUuid nodeUUID = sendingNode->getUUID();

        // now that we know the node ID, let's add these stats to the stats for that node...
        _octreeSceneStatsLock.lockForWrite();
        if (_octreeServerSceneStats.find(nodeUUID) != _octreeServerSceneStats.end()) {
            _octreeServerSceneStats[nodeUUID].unpackFromMessage(reinterpret_cast<const unsigned char*>(packet.data()),
                                                                packet.size());
        } else {
            _octreeServerSceneStats[nodeUUID] = temp;
        }
        _octreeSceneStatsLock.unlock();

        VoxelPositionSize rootDetails;
        voxelDetailsForCode(temp.getJurisdictionRoot(), rootDetails);

        // see if this is the first we've heard of this node...
        NodeToJurisdictionMap* jurisdiction = NULL;
        QString serverType;
        if (sendingNode->getType() == NodeType::EntityServer) {
            jurisdiction = &_entityServerJurisdictions;
            serverType = "Entity";
        }

        jurisdiction->lockForRead();
        if (jurisdiction->find(nodeUUID) == jurisdiction->end()) {
            jurisdiction->unlock();

            qCDebug(interfaceapp, "stats from new %s server... [%f, %f, %f, %f]",
                    qPrintable(serverType),
                    (double)rootDetails.x, (double)rootDetails.y, (double)rootDetails.z, (double)rootDetails.s);

            // Add the jurisditionDetails object to the list of "fade outs"
            if (!Menu::getInstance()->isOptionChecked(MenuOption::DontFadeOnOctreeServerChanges)) {
                OctreeFade fade(OctreeFade::FADE_OUT, NODE_ADDED_RED, NODE_ADDED_GREEN, NODE_ADDED_BLUE);
                fade.voxelDetails = rootDetails;
                const float slightly_smaller = 0.99f;
                fade.voxelDetails.s = fade.voxelDetails.s * slightly_smaller;
                _octreeFadesLock.lockForWrite();
                _octreeFades.push_back(fade);
                _octreeFadesLock.unlock();
            }
        } else {
            jurisdiction->unlock();
        }
        // store jurisdiction details for later use
        // This is bit of fiddling is because JurisdictionMap assumes it is the owner of the values used to construct it
        // but OctreeSceneStats thinks it's just returning a reference to its contents. So we need to make a copy of the
        // details from the OctreeSceneStats to construct the JurisdictionMap
        JurisdictionMap jurisdictionMap;
        jurisdictionMap.copyContents(temp.getJurisdictionRoot(), temp.getJurisdictionEndNodes());
        jurisdiction->lockForWrite();
        (*jurisdiction)[nodeUUID] = jurisdictionMap;
        jurisdiction->unlock();
    }
    return statsMessageLength;
}

void Application::packetSent(quint64 length) {
}

const QString SETTINGS_KEY = "Settings";

void Application::loadScripts() {
    // loads all saved scripts
    Settings settings;
    int size = settings.beginReadArray(SETTINGS_KEY);
    for (int i = 0; i < size; ++i){
        settings.setArrayIndex(i);
        QString string = settings.value("script").toString();
        if (!string.isEmpty()) {
            loadScript(string);
        }
    }
    settings.endArray();
}

void Application::clearScriptsBeforeRunning() {
    // clears all scripts from the settingsSettings settings;
    Settings settings;
    settings.beginWriteArray(SETTINGS_KEY);
    settings.remove("");
}

void Application::saveScripts() {
    // Saves all currently running user-loaded scripts
    Settings settings;
    settings.beginWriteArray(SETTINGS_KEY);
    settings.remove("");

    QStringList runningScripts = getRunningScripts();
    int i = 0;
    for (auto it = runningScripts.begin(); it != runningScripts.end(); ++it) {
        if (getScriptEngine(*it)->isUserLoaded()) {
            settings.setArrayIndex(i);
            settings.setValue("script", *it);
            ++i;
        }
    }
    settings.endArray();
}

QScriptValue joystickToScriptValue(QScriptEngine *engine, Joystick* const &in) {
    return engine->newQObject(in);
}

void joystickFromScriptValue(const QScriptValue &object, Joystick* &out) {
    out = qobject_cast<Joystick*>(object.toQObject());
}

void Application::registerScriptEngineWithApplicationServices(ScriptEngine* scriptEngine) {
    // setup the packet senders and jurisdiction listeners of the script engine's scripting interfaces so
    // we can use the same ones from the application.
    auto entityScriptingInterface = DependencyManager::get<EntityScriptingInterface>();
    entityScriptingInterface->setPacketSender(&_entityEditSender);
    entityScriptingInterface->setEntityTree(_entities.getTree());

    // AvatarManager has some custom types
    AvatarManager::registerMetaTypes(scriptEngine);

    // hook our avatar and avatar hash map object into this script engine
    scriptEngine->setAvatarData(_myAvatar, "MyAvatar"); // leave it as a MyAvatar class to expose thrust features
    scriptEngine->setAvatarHashMap(DependencyManager::get<AvatarManager>().data(), "AvatarList");

    scriptEngine->registerGlobalObject("Camera", &_myCamera);

#if defined(Q_OS_MAC) || defined(Q_OS_WIN)
    scriptEngine->registerGlobalObject("SpeechRecognizer", DependencyManager::get<SpeechRecognizer>().data());
#endif

    ClipboardScriptingInterface* clipboardScriptable = new ClipboardScriptingInterface();
    scriptEngine->registerGlobalObject("Clipboard", clipboardScriptable);
    connect(scriptEngine, SIGNAL(finished(const QString&)), clipboardScriptable, SLOT(deleteLater()));

    connect(scriptEngine, SIGNAL(finished(const QString&)), this, SLOT(scriptFinished(const QString&)));

    connect(scriptEngine, SIGNAL(loadScript(const QString&, bool)), this, SLOT(loadScript(const QString&, bool)));
    connect(scriptEngine, SIGNAL(reloadScript(const QString&, bool)), this, SLOT(reloadScript(const QString&, bool)));

    scriptEngine->registerGlobalObject("Overlays", &_overlays);
    qScriptRegisterMetaType(scriptEngine, OverlayPropertyResultToScriptValue, OverlayPropertyResultFromScriptValue);
    qScriptRegisterMetaType(scriptEngine, RayToOverlayIntersectionResultToScriptValue,
                            RayToOverlayIntersectionResultFromScriptValue);

    QScriptValue windowValue = scriptEngine->registerGlobalObject("Window", DependencyManager::get<WindowScriptingInterface>().data());
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                                       LocationScriptingInterface::locationSetter, windowValue);
    // register `location` on the global object.
    scriptEngine->registerGetterSetter("location", LocationScriptingInterface::locationGetter,
                                       LocationScriptingInterface::locationSetter);

    scriptEngine->registerFunction("WebWindow", WebWindowClass::constructor, 1);

    scriptEngine->registerGlobalObject("Menu", MenuScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("Settings", SettingsScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("AudioDevice", AudioDeviceScriptingInterface::getInstance());
    scriptEngine->registerGlobalObject("AnimationCache", DependencyManager::get<AnimationCache>().data());
    scriptEngine->registerGlobalObject("SoundCache", DependencyManager::get<SoundCache>().data());
    scriptEngine->registerGlobalObject("Account", AccountScriptingInterface::getInstance());

    scriptEngine->registerGlobalObject("GlobalServices", GlobalServicesScriptingInterface::getInstance());
    qScriptRegisterMetaType(scriptEngine, DownloadInfoResultToScriptValue, DownloadInfoResultFromScriptValue);

    scriptEngine->registerGlobalObject("AvatarManager", DependencyManager::get<AvatarManager>().data());

    qScriptRegisterMetaType(scriptEngine, joystickToScriptValue, joystickFromScriptValue);

    scriptEngine->registerGlobalObject("UndoStack", &_undoStackScriptingInterface);

    scriptEngine->registerGlobalObject("LODManager", DependencyManager::get<LODManager>().data());

    scriptEngine->registerGlobalObject("Paths", DependencyManager::get<PathUtils>().data());

    QScriptValue hmdInterface = scriptEngine->registerGlobalObject("HMD", &HMDScriptingInterface::getInstance());
    scriptEngine->registerFunction(hmdInterface, "getHUDLookAtPosition2D", HMDScriptingInterface::getHUDLookAtPosition2D, 0);
    scriptEngine->registerFunction(hmdInterface, "getHUDLookAtPosition3D", HMDScriptingInterface::getHUDLookAtPosition3D, 0);

    scriptEngine->registerGlobalObject("Scene", DependencyManager::get<SceneScriptingInterface>().data());

    scriptEngine->registerGlobalObject("ScriptDiscoveryService", this->getRunningScriptsWidget());

#ifdef HAVE_RTMIDI
    scriptEngine->registerGlobalObject("MIDI", &MIDIManager::getInstance());
#endif

    // TODO: Consider moving some of this functionality into the ScriptEngine class instead. It seems wrong that this
    // work is being done in the Application class when really these dependencies are more related to the ScriptEngine's
    // implementation
    QThread* workerThread = new QThread(this);
    QString scriptEngineName = QString("Script Thread:") + scriptEngine->getFilename();
    workerThread->setObjectName(scriptEngineName);

    // when the worker thread is started, call our engine's run..
    connect(workerThread, &QThread::started, scriptEngine, &ScriptEngine::run);

    // when the thread is terminated, add both scriptEngine and thread to the deleteLater queue
    connect(scriptEngine, SIGNAL(doneRunning()), scriptEngine, SLOT(deleteLater()));
    connect(workerThread, SIGNAL(finished()), workerThread, SLOT(deleteLater()));

    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &NodeList::nodeKilled, scriptEngine, &ScriptEngine::nodeKilled);

    scriptEngine->moveToThread(workerThread);

    // Starts an event loop, and emits workerThread->started()
    workerThread->start();
}

void Application::initializeAcceptedFiles() {
    if (_acceptedExtensions.size() == 0) {
        _acceptedExtensions[SNAPSHOT_EXTENSION] = &Application::acceptSnapshot;
        _acceptedExtensions[SVO_EXTENSION] = &Application::importSVOFromURL;
        _acceptedExtensions[SVO_JSON_EXTENSION] = &Application::importSVOFromURL;
        _acceptedExtensions[JS_EXTENSION] = &Application::askToLoadScript;
        _acceptedExtensions[FST_EXTENSION] = &Application::askToSetAvatarUrl;
    }
}

bool Application::canAcceptURL(const QString& urlString) {
    initializeAcceptedFiles();

    QUrl url(urlString);
    if (urlString.startsWith(HIFI_URL_SCHEME)) {
        return true;
    }
    QHashIterator<QString, AcceptURLMethod> i(_acceptedExtensions);
    QString lowerPath = url.path().toLower();
    while (i.hasNext()) {
        i.next();
        if (lowerPath.endsWith(i.key())) {
            return true;
        }
    }
    return false;
}

bool Application::acceptURL(const QString& urlString) {
    initializeAcceptedFiles();

    if (urlString.startsWith(HIFI_URL_SCHEME)) {
        // this is a hifi URL - have the AddressManager handle it
        QMetaObject::invokeMethod(DependencyManager::get<AddressManager>().data(), "handleLookupString",
                                  Qt::AutoConnection, Q_ARG(const QString&, urlString));
        return true;
    } else {
        QUrl url(urlString);
        QHashIterator<QString, AcceptURLMethod> i(_acceptedExtensions);
        QString lowerPath = url.path().toLower();
        while (i.hasNext()) {
            i.next();
            if (lowerPath.endsWith(i.key())) {
                AcceptURLMethod method = i.value();
                (this->*method)(urlString);
                return true;
            }
        }
    }
    return false;
}

void Application::setSessionUUID(const QUuid& sessionUUID) {
    _physicsEngine.setSessionUUID(sessionUUID);
}

bool Application::askToSetAvatarUrl(const QString& url) {
    QUrl realUrl(url);
    if (realUrl.isLocalFile()) {
        QString message = "You can not use local files for avatar components.";

        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return false;
    }

    // Download the FST file, to attempt to determine its model type
    QVariantHash fstMapping = FSTReader::downloadMapping(url);

    FSTReader::ModelType modelType = FSTReader::predictModelType(fstMapping);

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("Set Avatar");
    QPushButton* headButton = NULL;
    QPushButton* bodyButton = NULL;
    QPushButton* bodyAndHeadButton = NULL;

    QString modelName = fstMapping["name"].toString();
    QString message;
    QString typeInfo;
    switch (modelType) {
        case FSTReader::HEAD_MODEL:
            message = QString("Would you like to use '") + modelName + QString("' for your avatar head?");
            headButton = msgBox.addButton(tr("Yes"), QMessageBox::ActionRole);
        break;

        case FSTReader::BODY_ONLY_MODEL:
            message = QString("Would you like to use '") + modelName + QString("' for your avatar body?");
            bodyButton = msgBox.addButton(tr("Yes"), QMessageBox::ActionRole);
        break;

        case FSTReader::HEAD_AND_BODY_MODEL:
            message = QString("Would you like to use '") + modelName + QString("' for your avatar?");
            bodyAndHeadButton = msgBox.addButton(tr("Yes"), QMessageBox::ActionRole);
        break;

        default:
            message = QString("Would you like to use '") + modelName + QString("' for some part of your avatar head?");
            headButton = msgBox.addButton(tr("Use for Head"), QMessageBox::ActionRole);
            bodyButton = msgBox.addButton(tr("Use for Body"), QMessageBox::ActionRole);
            bodyAndHeadButton = msgBox.addButton(tr("Use for Body and Head"), QMessageBox::ActionRole);
        break;
    }

    msgBox.setText(message);
    msgBox.addButton(QMessageBox::Cancel);

    msgBox.exec();

    if (msgBox.clickedButton() == headButton) {
        _myAvatar->useHeadURL(url, modelName);
        emit headURLChanged(url, modelName);
    } else if (msgBox.clickedButton() == bodyButton) {
        _myAvatar->useBodyURL(url, modelName);
        emit bodyURLChanged(url, modelName);
    } else if (msgBox.clickedButton() == bodyAndHeadButton) {
        _myAvatar->useFullAvatarURL(url, modelName);
        emit fullAvatarURLChanged(url, modelName);
    } else {
        qCDebug(interfaceapp) << "Declined to use the avatar: " << url;
    }

    return true;
}


bool Application::askToLoadScript(const QString& scriptFilenameOrURL) {
    QMessageBox::StandardButton reply;
    QString message = "Would you like to run this script:\n" + scriptFilenameOrURL;
    reply = QMessageBox::question(getWindow(), "Run Script", message, QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        qCDebug(interfaceapp) << "Chose to run the script: " << scriptFilenameOrURL;
        loadScript(scriptFilenameOrURL);
    } else {
        qCDebug(interfaceapp) << "Declined to run the script: " << scriptFilenameOrURL;
    }
    return true;
}

ScriptEngine* Application::loadScript(const QString& scriptFilename, bool isUserLoaded,
                                        bool loadScriptFromEditor, bool activateMainWindow, bool reload) {

    if (isAboutToQuit()) {
        return NULL;
    }

    QUrl scriptUrl(scriptFilename);
    const QString& scriptURLString = scriptUrl.toString();
    if (_scriptEnginesHash.contains(scriptURLString) && loadScriptFromEditor
        && !_scriptEnginesHash[scriptURLString]->isFinished()) {

        return _scriptEnginesHash[scriptURLString];
    }

    ScriptEngine* scriptEngine = new ScriptEngine(NO_SCRIPT, "", &_controllerScriptingInterface);
    scriptEngine->setUserLoaded(isUserLoaded);

    if (scriptFilename.isNull()) {
        // this had better be the script editor (we should de-couple so somebody who thinks they are loading a script
        // doesn't just get an empty script engine)

        // we can complete setup now since there isn't a script we have to load
        registerScriptEngineWithApplicationServices(scriptEngine);
    } else {
        // connect to the appropriate signals of this script engine
        connect(scriptEngine, &ScriptEngine::scriptLoaded, this, &Application::handleScriptEngineLoaded);
        connect(scriptEngine, &ScriptEngine::errorLoadingScript, this, &Application::handleScriptLoadError);

        // get the script engine object to load the script at the designated script URL
        scriptEngine->loadURL(scriptUrl, reload);
    }

    // restore the main window's active state
    if (activateMainWindow && !loadScriptFromEditor) {
        _window->activateWindow();
    }

    return scriptEngine;
}

void Application::reloadScript(const QString& scriptName, bool isUserLoaded) {
    loadScript(scriptName, isUserLoaded, false, false, true);
}

void Application::handleScriptEngineLoaded(const QString& scriptFilename) {
    ScriptEngine* scriptEngine = qobject_cast<ScriptEngine*>(sender());

    _scriptEnginesHash.insertMulti(scriptFilename, scriptEngine);
    _runningScriptsWidget->setRunningScripts(getRunningScripts());
    UserActivityLogger::getInstance().loadedScript(scriptFilename);

    // register our application services and set it off on its own thread
    registerScriptEngineWithApplicationServices(scriptEngine);
}

void Application::handleScriptLoadError(const QString& scriptFilename) {
    qCDebug(interfaceapp) << "Application::loadScript(), script failed to load...";
    QMessageBox::warning(getWindow(), "Error Loading Script", scriptFilename + " failed to load.");
}

void Application::scriptFinished(const QString& scriptName) {
    const QString& scriptURLString = QUrl(scriptName).toString();
    QHash<QString, ScriptEngine*>::iterator it = _scriptEnginesHash.find(scriptURLString);
    if (it != _scriptEnginesHash.end()) {
        _scriptEnginesHash.erase(it);
        _runningScriptsWidget->scriptStopped(scriptName);
        _runningScriptsWidget->setRunningScripts(getRunningScripts());
    }
}

void Application::stopAllScripts(bool restart) {
    if (restart) {
        // Delete all running scripts from cache so that they are re-downloaded when they are restarted
        auto scriptCache = DependencyManager::get<ScriptCache>();
        for (QHash<QString, ScriptEngine*>::const_iterator it = _scriptEnginesHash.constBegin();
            it != _scriptEnginesHash.constEnd(); it++) {
            if (!it.value()->isFinished()) {
                scriptCache->deleteScript(it.key());
            }
        }
    }

    // Stop and possibly restart all currently running scripts
    for (QHash<QString, ScriptEngine*>::const_iterator it = _scriptEnginesHash.constBegin();
            it != _scriptEnginesHash.constEnd(); it++) {
        if (it.value()->isFinished()) {
            continue;
        }
        if (restart && it.value()->isUserLoaded()) {
            connect(it.value(), SIGNAL(finished(const QString&)), SLOT(reloadScript(const QString&)));
        }
        it.value()->stop();
        qCDebug(interfaceapp) << "stopping script..." << it.key();
    }
    // HACK: ATM scripts cannot set/get their animation priorities, so we clear priorities
    // whenever a script stops in case it happened to have been setting joint rotations.
    // TODO: expose animation priorities and provide a layered animation control system.
    _myAvatar->clearJointAnimationPriorities();
    _myAvatar->clearScriptableSettings();
}

void Application::stopScript(const QString &scriptName, bool restart) {
    const QString& scriptURLString = QUrl(scriptName).toString();
    if (_scriptEnginesHash.contains(scriptURLString)) {
        ScriptEngine* scriptEngine = _scriptEnginesHash[scriptURLString];
        if (restart) {
            auto scriptCache = DependencyManager::get<ScriptCache>();
            scriptCache->deleteScript(scriptName);
            connect(scriptEngine, SIGNAL(finished(const QString&)), SLOT(reloadScript(const QString&)));
        }
        scriptEngine->stop();
        qCDebug(interfaceapp) << "stopping script..." << scriptName;
        // HACK: ATM scripts cannot set/get their animation priorities, so we clear priorities
        // whenever a script stops in case it happened to have been setting joint rotations.
        // TODO: expose animation priorities and provide a layered animation control system.
        _myAvatar->clearJointAnimationPriorities();
    }
    if (_scriptEnginesHash.empty()) {
        _myAvatar->clearScriptableSettings();
    }
}

void Application::reloadAllScripts() {
    stopAllScripts(true);
}

void Application::reloadOneScript(const QString& scriptName) {
    stopScript(scriptName, true);
}

void Application::loadDefaultScripts() {
    if (!_scriptEnginesHash.contains(DEFAULT_SCRIPTS_JS_URL)) {
        loadScript(DEFAULT_SCRIPTS_JS_URL);
    }
}

void Application::manageRunningScriptsWidgetVisibility(bool shown) {
    if (_runningScriptsWidgetWasVisible && shown) {
        _runningScriptsWidget->show();
    } else if (_runningScriptsWidgetWasVisible && !shown) {
        _runningScriptsWidget->hide();
    }
}

void Application::toggleRunningScriptsWidget() {
    if (_runningScriptsWidget->isVisible()) {
        if (_runningScriptsWidget->hasFocus()) {
            _runningScriptsWidget->hide();
        } else {
            _runningScriptsWidget->raise();
            setActiveWindow(_runningScriptsWidget);
            _runningScriptsWidget->setFocus();
        }
    } else {
        _runningScriptsWidget->show();
        _runningScriptsWidget->setFocus();
    }
}

void Application::packageModel() {
    ModelPackager::package();
}

void Application::openUrl(const QUrl& url) {
    if (!url.isEmpty()) {
        if (url.scheme() == HIFI_URL_SCHEME) {
            DependencyManager::get<AddressManager>()->handleLookupString(url.toString());
        } else {
            // address manager did not handle - ask QDesktopServices to handle
            QDesktopServices::openUrl(url);
        }
    }
}

void Application::updateMyAvatarTransform() {
    const float SIMULATION_OFFSET_QUANTIZATION = 16.0f; // meters
    glm::vec3 avatarPosition = _myAvatar->getPosition();
    glm::vec3 physicsWorldOffset = _physicsEngine.getOriginOffset();
    if (glm::distance(avatarPosition, physicsWorldOffset) > SIMULATION_OFFSET_QUANTIZATION) {
        glm::vec3 newOriginOffset = avatarPosition;
        int halfExtent = (int)HALF_SIMULATION_EXTENT;
        for (int i = 0; i < 3; ++i) {
            newOriginOffset[i] = (float)(glm::max(halfExtent,
                    ((int)(avatarPosition[i] / SIMULATION_OFFSET_QUANTIZATION)) * (int)SIMULATION_OFFSET_QUANTIZATION));
        }
        // TODO: Andrew to replace this with method that actually moves existing object positions in PhysicsEngine
        _physicsEngine.setOriginOffset(newOriginOffset);
    }
}

void Application::domainSettingsReceived(const QJsonObject& domainSettingsObject) {
    // from the domain-handler, figure out the satoshi cost per voxel and per meter cubed
    const QString VOXEL_SETTINGS_KEY = "voxels";
    const QString PER_VOXEL_COST_KEY = "per-voxel-credits";
    const QString PER_METER_CUBED_COST_KEY = "per-meter-cubed-credits";
    const QString VOXEL_WALLET_UUID = "voxel-wallet";

    const QJsonObject& voxelObject = domainSettingsObject[VOXEL_SETTINGS_KEY].toObject();

    qint64 satoshisPerVoxel = 0;
    qint64 satoshisPerMeterCubed = 0;
    QUuid voxelWalletUUID;

    if (!domainSettingsObject.isEmpty()) {
        float perVoxelCredits = (float) voxelObject[PER_VOXEL_COST_KEY].toDouble();
        float perMeterCubedCredits = (float) voxelObject[PER_METER_CUBED_COST_KEY].toDouble();

        satoshisPerVoxel = (qint64) floorf(perVoxelCredits * SATOSHIS_PER_CREDIT);
        satoshisPerMeterCubed = (qint64) floorf(perMeterCubedCredits * SATOSHIS_PER_CREDIT);

        voxelWalletUUID = QUuid(voxelObject[VOXEL_WALLET_UUID].toString());
    }

    qCDebug(interfaceapp) << "Octree edits costs are" << satoshisPerVoxel << "per octree cell and" << satoshisPerMeterCubed << "per meter cubed";
    qCDebug(interfaceapp) << "Destination wallet UUID for edit payments is" << voxelWalletUUID;
}

QString Application::getPreviousScriptLocation() {
    QString suggestedName;
    if (_previousScriptLocation.get().isEmpty()) {
        QString desktopLocation = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
// Temporary fix to Qt bug: http://stackoverflow.com/questions/16194475
#ifdef __APPLE__
        suggestedName = desktopLocation.append("/script.js");
#endif
    } else {
        suggestedName = _previousScriptLocation.get();
    }
    return suggestedName;
}

void Application::setPreviousScriptLocation(const QString& previousScriptLocation) {
    _previousScriptLocation.set(previousScriptLocation);
}

void Application::loadDialog() {

    QString fileNameString = QFileDialog::getOpenFileName(_glWidget,
                                                          tr("Open Script"),
                                                          getPreviousScriptLocation(),
                                                          tr("JavaScript Files (*.js)"));
    if (!fileNameString.isEmpty()) {
        setPreviousScriptLocation(fileNameString);
        loadScript(fileNameString);
    }
}

void Application::loadScriptURLDialog() {
    QInputDialog scriptURLDialog(Application::getInstance()->getWindow());
    scriptURLDialog.setWindowTitle("Open and Run Script URL");
    scriptURLDialog.setLabelText("Script:");
    scriptURLDialog.setWindowFlags(Qt::Sheet);
    const float DIALOG_RATIO_OF_WINDOW = 0.30f;
    scriptURLDialog.resize(scriptURLDialog.parentWidget()->size().width() * DIALOG_RATIO_OF_WINDOW,
                        scriptURLDialog.size().height());

    int dialogReturn = scriptURLDialog.exec();
    QString newScript;
    if (dialogReturn == QDialog::Accepted) {
        if (scriptURLDialog.textValue().size() > 0) {
            // the user input a new hostname, use that
            newScript = scriptURLDialog.textValue();
        }
        loadScript(newScript);
    }
}

QString Application::getScriptsLocation() {
    return _scriptsLocationHandle.get();
}

void Application::setScriptsLocation(const QString& scriptsLocation) {
    _scriptsLocationHandle.set(scriptsLocation);
    emit scriptLocationChanged(scriptsLocation);
}

void Application::toggleLogDialog() {
    if (! _logDialog) {
        _logDialog = new LogDialog(_glWidget, getLogger());
    }

    if (_logDialog->isVisible()) {
        _logDialog->hide();
    } else {
        _logDialog->show();
    }
}

void Application::takeSnapshot() {
    QMediaPlayer* player = new QMediaPlayer();
    QFileInfo inf = QFileInfo(PathUtils::resourcesPath() + "sounds/snap.wav");
    player->setMedia(QUrl::fromLocalFile(inf.absoluteFilePath()));
    player->play();

    QString fileName = Snapshot::saveSnapshot(_glWidget->grabFrameBuffer());

    AccountManager& accountManager = AccountManager::getInstance();
    if (!accountManager.isLoggedIn()) {
        return;
    }

    if (!_snapshotShareDialog) {
        _snapshotShareDialog = new SnapshotShareDialog(fileName, _glWidget);
    }
    _snapshotShareDialog->show();
}

void Application::setVSyncEnabled() {
#if defined(Q_OS_WIN)
    bool vsyncOn = Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerateVSyncOn);
    if (wglewGetExtension("WGL_EXT_swap_control")) {
        wglSwapIntervalEXT(vsyncOn);
        int swapInterval = wglGetSwapIntervalEXT();
        qCDebug(interfaceapp, "V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    } else {
        qCDebug(interfaceapp, "V-Sync is FORCED ON on this system\n");
    }
#elif defined(Q_OS_LINUX)
    // TODO: write the poper code for linux
    /*
    if (glQueryExtension.... ("GLX_EXT_swap_control")) {
        glxSwapIntervalEXT(vsyncOn);
        int swapInterval = xglGetSwapIntervalEXT();
        _isVSyncOn = swapInterval;
        qCDebug(interfaceapp, "V-Sync is %s\n", (swapInterval > 0 ? "ON" : "OFF"));
    } else {
    qCDebug(interfaceapp, "V-Sync is FORCED ON on this system\n");
    }
    */
#else
    qCDebug(interfaceapp, "V-Sync is FORCED ON on this system\n");
#endif
}

bool Application::isVSyncOn() const {
#if defined(Q_OS_WIN)
    if (wglewGetExtension("WGL_EXT_swap_control")) {
        int swapInterval = wglGetSwapIntervalEXT();
        return (swapInterval > 0);
    }
#elif defined(Q_OS_LINUX)
    // TODO: write the poper code for linux
    /*
    if (glQueryExtension.... ("GLX_EXT_swap_control")) {
        int swapInterval = xglGetSwapIntervalEXT();
        return (swapInterval > 0);
    } else {
        return true;
    }
    */
#endif
    return true;
}

bool Application::isVSyncEditable() const {
#if defined(Q_OS_WIN)
    if (wglewGetExtension("WGL_EXT_swap_control")) {
        return true;
    }
#elif defined(Q_OS_LINUX)
    // TODO: write the poper code for linux
    /*
    if (glQueryExtension.... ("GLX_EXT_swap_control")) {
        return true;
    }
    */
#endif
    return false;
}

unsigned int Application::getRenderTargetFramerate() const {
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerateUnlimited)) {
        return 0;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerate60)) {
        return 60;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerate50)) {
        return 50;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerate40)) {
        return 40;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderTargetFramerate30)) {
        return 30;
    }
    return 0;
}

float Application::getRenderResolutionScale() const {
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionOne)) {
        return 1.0f;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionTwoThird)) {
        return 0.666f;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionHalf)) {
        return 0.5f;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionThird)) {
        return 0.333f;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderResolutionQuarter)) {
        return 0.25f;
    } else {
        return 1.0f;
    }
}

int Application::getRenderAmbientLight() const {
    if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLightGlobal)) {
        return -1;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight0)) {
        return 0;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight1)) {
        return 1;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight2)) {
        return 2;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight3)) {
        return 3;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight4)) {
        return 4;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight5)) {
        return 5;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight6)) {
        return 6;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight7)) {
        return 7;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight8)) {
        return 8;
    } else if (Menu::getInstance()->isOptionChecked(MenuOption::RenderAmbientLight9)) {
        return 9;
    } else {
        return -1;
    }
}

void Application::notifyPacketVersionMismatch() {
    if (!_notifiedPacketVersionMismatchThisDomain) {
        _notifiedPacketVersionMismatchThisDomain = true;

        QString message = "The location you are visiting is running an incompatible server version.\n";
        message += "Content may not display properly.";

        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
    }
}

void Application::checkSkeleton() {
    if (_myAvatar->getSkeletonModel().isActive() && !_myAvatar->getSkeletonModel().hasSkeleton()) {
        qCDebug(interfaceapp) << "MyAvatar model has no skeleton";

        QString message = "Your selected avatar body has no skeleton.\n\nThe default body will be loaded...";
        QMessageBox msgBox;
        msgBox.setText(message);
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();

        _myAvatar->useBodyURL(DEFAULT_BODY_MODEL_URL, "Default");
    } else {
        _physicsEngine.setCharacterController(_myAvatar->getCharacterController());
    }
}

void Application::showFriendsWindow() {
    const QString FRIENDS_WINDOW_TITLE = "Add/Remove Friends";
    const QString FRIENDS_WINDOW_URL = "https://metaverse.highfidelity.com/user/friends";
    const int FRIENDS_WINDOW_WIDTH = 290;
    const int FRIENDS_WINDOW_HEIGHT = 500;
    if (!_friendsWindow) {
        _friendsWindow = new WebWindowClass(FRIENDS_WINDOW_TITLE, FRIENDS_WINDOW_URL, FRIENDS_WINDOW_WIDTH,
            FRIENDS_WINDOW_HEIGHT, false);
        connect(_friendsWindow, &WebWindowClass::closed, this, &Application::friendsWindowClosed);
    }
    _friendsWindow->setVisible(true);
}

void Application::friendsWindowClosed() {
    delete _friendsWindow;
    _friendsWindow = NULL;
}

void Application::postLambdaEvent(std::function<void()> f) {
    QCoreApplication::postEvent(this, new LambdaEvent(f));
}

void Application::initPlugins() {
}

void Application::shutdownPlugins() {
}

glm::vec3 Application::getHeadPosition() const {
    return OculusManager::getRelativePosition();
}


glm::quat Application::getHeadOrientation() const {
    return OculusManager::getOrientation();
}

glm::uvec2 Application::getCanvasSize() const {
    return glm::uvec2(_glWidget->width(), _glWidget->height());
}

QSize Application::getDeviceSize() const {
    return _glWidget->getDeviceSize();
}

ivec2 Application::getTrueMouse() const {
    return toGlm(_glWidget->mapFromGlobal(QCursor::pos()));
}

bool Application::isThrottleRendering() const {
    return _glWidget->isThrottleRendering();
}

PickRay Application::computePickRay() const {
    return computePickRay(getTrueMouseX(), getTrueMouseY());
}

bool Application::hasFocus() const {
    return _glWidget->hasFocus();
}

void Application::setMaxOctreePacketsPerSecond(int maxOctreePPS) {
    if (maxOctreePPS != _maxOctreePPS) {
        _maxOctreePPS = maxOctreePPS;
        maxOctreePacketsPerSecond.set(_maxOctreePPS);
    }
}

int Application::getMaxOctreePacketsPerSecond() {
    return _maxOctreePPS;
}

qreal Application::getDevicePixelRatio() {
    return _window ? _window->windowHandle()->devicePixelRatio() : 1.0;
}

mat4 Application::getEyeProjection(int eye) const {
    if (isHMDMode()) {
        return OculusManager::getEyeProjection(eye);
    } 
      
    return _viewFrustum.getProjection();
}

mat4 Application::getEyePose(int eye) const {
    if (isHMDMode()) {
        return OculusManager::getEyePose(eye);
    }

    return mat4();
}

mat4 Application::getHeadPose() const {
    if (isHMDMode()) {
        return OculusManager::getHeadPose();
    }
    return mat4();
}
