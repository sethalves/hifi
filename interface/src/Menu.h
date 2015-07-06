//
//  Menu.h
//  interface/src
//
//  Created by Stephen Birarda on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Menu_h
#define hifi_Menu_h

#include <QDir>
#include <QMenuBar>
#include <QHash>
#include <QKeySequence>
#include <QPointer>
#include <QStandardPaths>

#include <MenuItemProperties.h>

#include "DiscoverabilityManager.h"

class Settings;

class MenuWrapper : public QObject {
public:
    QList<QAction*> actions();
    MenuWrapper* addMenu(const QString& menuName);
    void setEnabled(bool enabled = true);
    void addSeparator();
    void addAction(QAction* action);

    QAction* addAction(const QString& menuName);
    void insertAction(QAction* before, QAction* menuName);

    QAction* addAction(const QString& menuName, const QObject* receiver, const char* member, const QKeySequence& shortcut = 0);
    void removeAction(QAction* action);

    QAction* newAction() {
        return new QAction(_realMenu);
    }
private:
    MenuWrapper(QMenu* menu);

    static MenuWrapper* fromMenu(QMenu* menu) {
        return _backMap[menu];
    }

    QMenu* const _realMenu;
    static QHash<QMenu*, MenuWrapper*> _backMap;
    friend class Menu;
};

class Menu : public QMenuBar {
    Q_OBJECT
public:
    static Menu* getInstance();

    void loadSettings();
    void saveSettings();

    MenuWrapper* getMenu(const QString& menuName);

    void triggerOption(const QString& menuOption);
    QAction* getActionForOption(const QString& menuOption);

    QAction* addActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                           const QString& actionName,
                                           const QKeySequence& shortcut = 0,
                                           const QObject* receiver = NULL,
                                           const char* member = NULL,
                                           QAction::MenuRole role = QAction::NoRole,
                                           int menuItemLocation = UNSPECIFIED_POSITION);
    QAction* addActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                           QAction* action,
                                           const QString& actionName = QString(),
                                           const QKeySequence& shortcut = 0,
                                           QAction::MenuRole role = QAction::NoRole,
                                           int menuItemLocation = UNSPECIFIED_POSITION);

    void removeAction(MenuWrapper* menu, const QString& actionName);

public slots:
    MenuWrapper* addMenu(const QString& menuName);
    void removeMenu(const QString& menuName);
    bool menuExists(const QString& menuName);
    void addSeparator(const QString& menuName, const QString& separatorName);
    void removeSeparator(const QString& menuName, const QString& separatorName);
    void addMenuItem(const MenuItemProperties& properties);
    void removeMenuItem(const QString& menuName, const QString& menuitem);
    bool menuItemExists(const QString& menuName, const QString& menuitem);
    bool isOptionChecked(const QString& menuOption) const;
    void setIsOptionChecked(const QString& menuOption, bool isChecked);

private:
    static Menu* _instance;
    Menu();

    typedef void(*settingsAction)(Settings&, QAction&);
    static void loadAction(Settings& settings, QAction& action);
    static void saveAction(Settings& settings, QAction& action);
    void scanMenuBar(settingsAction modifySetting);
    void scanMenu(QMenu& menu, settingsAction modifySetting, Settings& settings);

    /// helper method to have separators with labels that are also compatible with OS X
    void addDisabledActionAndSeparator(MenuWrapper* destinationMenu, const QString& actionName,
                                       int menuItemLocation = UNSPECIFIED_POSITION);

    QAction* addCheckableActionToQMenuAndActionHash(MenuWrapper* destinationMenu,
                                                    const QString& actionName,
                                                    const QKeySequence& shortcut = 0,
                                                    const bool checked = false,
                                                    const QObject* receiver = NULL,
                                                    const char* member = NULL,
                                                    int menuItemLocation = UNSPECIFIED_POSITION);

    QAction* getActionFromName(const QString& menuName, MenuWrapper* menu);
    MenuWrapper* getSubMenuFromName(const QString& menuName, MenuWrapper* menu);
    MenuWrapper* getMenuParent(const QString& menuName, QString& finalMenuPart);

    QAction* getMenuAction(const QString& menuName);
    int findPositionOfMenuItem(MenuWrapper* menu, const QString& searchMenuItem);
    int positionBeforeSeparatorIfNeeded(MenuWrapper* menu, int requestedPosition);

    QHash<QString, QAction*> _actionHash;
};

namespace MenuOption {
    const QString AboutApp = "About Interface";
    const QString AddRemoveFriends = "Add/Remove Friends...";
    const QString AddressBar = "Show Address Bar";
    const QString AlignForearmsWithWrists = "Align Forearms with Wrists";
    const QString AlternateIK = "Alternate IK";
    const QString AmbientOcclusion = "Ambient Occlusion";
    const QString Animations = "Animations...";
    const QString Atmosphere = "Atmosphere";
    const QString Attachments = "Attachments...";
    const QString AudioNoiseReduction = "Audio Noise Reduction";
    const QString AudioScope = "Show Scope";
    const QString AudioScopeFiftyFrames = "Fifty";
    const QString AudioScopeFiveFrames = "Five";
    const QString AudioScopeFrames = "Display Frames";
    const QString AudioScopePause = "Pause Scope";
    const QString AudioScopeTwentyFrames = "Twenty";
    const QString AudioStats = "Audio Stats";
    const QString AudioStatsShowInjectedStreams = "Audio Stats Show Injected Streams";
    const QString AutoMuteAudio = "Auto Mute Microphone";
    const QString AvatarReceiveStats = "Show Receive Stats";
    const QString Back = "Back";
    const QString BandwidthDetails = "Bandwidth Details";
    const QString BinaryEyelidControl = "Binary Eyelid Control";
    const QString BlueSpeechSphere = "Blue Sphere While Speaking";
    const QString BookmarkLocation = "Bookmark Location";
    const QString Bookmarks = "Bookmarks";
    const QString CascadedShadows = "Cascaded";
    const QString CachesSize = "RAM Caches Size";
    const QString CalibrateCamera = "Calibrate Camera";
    const QString CenterPlayerInView = "Center Player In View";
    const QString Chat = "Chat...";
    const QString Collisions = "Collisions";
    const QString Console = "Console...";
    const QString ControlWithSpeech = "Control With Speech";
    const QString CopyAddress = "Copy Address to Clipboard";
    const QString CopyPath = "Copy Path to Clipboard";
    const QString DecreaseAvatarSize = "Decrease Avatar Size";
    const QString DeleteBookmark = "Delete Bookmark...";
    const QString DisableActivityLogger = "Disable Activity Logger";
    const QString DisableLightEntities = "Disable Light Entities";
    const QString DisableNackPackets = "Disable NACK Packets";
    const QString DiskCacheEditor = "Disk Cache Editor";
    const QString DisplayHands = "Show Hand Info";
    const QString DisplayHandTargets = "Show Hand Targets";
    const QString DisplayModelBounds = "Display Model Bounds";
    const QString DisplayModelTriangles = "Display Model Triangles";
    const QString DisplayModelElementChildProxies = "Display Model Element Children";
    const QString DisplayModelElementProxy = "Display Model Element Bounds";
    const QString DisplayDebugTimingDetails = "Display Timing Details";
    const QString DontDoPrecisionPicking = "Don't Do Precision Picking";
    const QString DontFadeOnOctreeServerChanges = "Don't Fade In/Out on Octree Server Changes";
    const QString DontRenderEntitiesAsScene = "Don't Render Entities as Scene";
    const QString EchoLocalAudio = "Echo Local Audio";
    const QString EchoServerAudio = "Echo Server Audio";
    const QString EditEntitiesHelp = "Edit Entities Help...";
    const QString Enable3DTVMode = "Enable 3DTV Mode";
    const QString EnableCharacterController = "Enable avatar collisions";
    const QString EnableGlowEffect = "Enable Glow Effect";
    const QString EnableVRMode = "Enable VR Mode";
    const QString ExpandMyAvatarSimulateTiming = "Expand /myAvatar/simulation";
    const QString ExpandMyAvatarTiming = "Expand /myAvatar";
    const QString ExpandOtherAvatarTiming = "Expand /otherAvatar";
    const QString ExpandPaintGLTiming = "Expand /paintGL";
    const QString ExpandUpdateTiming = "Expand /update";
    const QString Faceshift = "Faceshift";
    const QString FilterSixense = "Smooth Sixense Movement";
    const QString FirstPerson = "First Person";
    const QString Forward = "Forward";
    const QString FrameTimer = "Show Timer";
    const QString Fullscreen = "Fullscreen";
    const QString FullscreenMirror = "Fullscreen Mirror";
    const QString GlowWhenSpeaking = "Glow When Speaking";
    const QString HMDTools = "HMD Tools";
    const QString IncreaseAvatarSize = "Increase Avatar Size";
    const QString IndependentMode = "Independent Mode";
    const QString KeyboardMotorControl = "Enable Keyboard Motor Control";
    const QString LeapMotionOnHMD = "Leap Motion on HMD";
    const QString LoadScript = "Open and Run Script File...";
    const QString LoadScriptURL = "Open and Run Script from URL...";
    const QString LoadRSSDKFile = "Load .rssdk file";
    const QString LodTools = "LOD Tools";
    const QString Login = "Login";
    const QString Log = "Log";
    const QString LogExtraTimings = "Log Extra Timing Details";
    const QString LowVelocityFilter = "Low Velocity Filter";
    const QString Mirror = "Mirror";
    const QString MuteAudio = "Mute Microphone";
    const QString MuteEnvironment = "Mute Environment";
    const QString MuteFaceTracking = "Mute Face Tracking";
    const QString NamesAboveHeads = "Names Above Heads";
    const QString NoFaceTracking = "None";
    const QString OctreeStats = "Entity Statistics";
    const QString OnlyDisplayTopTen = "Only Display Top Ten";
    const QString PackageModel = "Package Model...";
    const QString Pair = "Pair";
    const QString PhysicsShowOwned = "Highlight Simulation Ownership";
    const QString PhysicsShowHulls = "Draw Collision Hulls";
    const QString PipelineWarnings = "Log Render Pipeline Warnings";
    const QString Preferences = "Preferences...";
    const QString Quit =  "Quit";
    const QString ReloadAllScripts = "Reload All Scripts";
    const QString RenderBoundingCollisionShapes = "Show Bounding Collision Shapes";
    const QString RenderFocusIndicator = "Show Eye Focus";
    const QString RenderHeadCollisionShapes = "Show Head Collision Shapes";
    const QString RenderLookAtVectors = "Show Look-at Vectors";
    const QString RenderSkeletonCollisionShapes = "Show Skeleton Collision Shapes";
    const QString RenderTargetFramerate = "Framerate";
    const QString RenderTargetFramerateUnlimited = "Unlimited";
    const QString RenderTargetFramerate60 = "60";
    const QString RenderTargetFramerate50 = "50";
    const QString RenderTargetFramerate40 = "40";
    const QString RenderTargetFramerate30 = "30";
    const QString RenderTargetFramerateVSyncOn = "V-Sync On";
    const QString RenderResolution = "Scale Resolution";
    const QString RenderResolutionOne = "1";
    const QString RenderResolutionTwoThird = "2/3";
    const QString RenderResolutionHalf = "1/2";
    const QString RenderResolutionThird = "1/3";
    const QString RenderResolutionQuarter = "1/4";
    const QString RenderAmbientLight = "Ambient Light";
    const QString RenderAmbientLightGlobal = "Global";
    const QString RenderAmbientLight0 = "OLD_TOWN_SQUARE";
    const QString RenderAmbientLight1 = "GRACE_CATHEDRAL";
    const QString RenderAmbientLight2 = "EUCALYPTUS_GROVE";
    const QString RenderAmbientLight3 = "ST_PETERS_BASILICA";
    const QString RenderAmbientLight4 = "UFFIZI_GALLERY";
    const QString RenderAmbientLight5 = "GALILEOS_TOMB";
    const QString RenderAmbientLight6 = "VINE_STREET_KITCHEN";
    const QString RenderAmbientLight7 = "BREEZEWAY";
    const QString RenderAmbientLight8 = "CAMPUS_SUNSET";
    const QString RenderAmbientLight9 = "FUNSTON_BEACH_SUNSET";
    const QString ResetAvatarSize = "Reset Avatar Size";
    const QString ResetSensors = "Reset Sensors";
    const QString RunningScripts = "Running Scripts";
    const QString RunTimingTests = "Run Timing Tests";
    const QString ScriptEditor = "Script Editor...";
    const QString ScriptedMotorControl = "Enable Scripted Motor Control";
    const QString ShowDSConnectTable = "Show Domain Connection Timing";
    const QString ShowBordersEntityNodes = "Show Entity Nodes";
    const QString ShowIKConstraints = "Show IK Constraints";
    const QString ShowRealtimeEntityStats = "Show Realtime Entity Stats";
    const QString SimpleShadows = "Simple";
    const QString SixenseEnabled = "Enable Hydra Support";
    const QString SixenseMouseInput = "Enable Sixense Mouse Input";
    const QString SixenseLasers = "Enable Sixense UI Lasers";
    const QString ShiftHipsForIdleAnimations = "Shift hips for idle animations";
    const QString Stars = "Stars";
    const QString Stats = "Stats";
    const QString StopAllScripts = "Stop All Scripts";
    const QString SuppressShortTimings = "Suppress Timings Less than 10ms";
    const QString TestPing = "Test Ping";
    const QString ThirdPerson = "Third Person";
    const QString ToolWindow = "Tool Window";
    const QString TransmitterDrive = "Transmitter Drive";
    const QString TurnWithHead = "Turn using Head";
    const QString UseAudioForMouth = "Use Audio for Mouth";
    const QString UseCamera = "Use Camera";
    const QString VelocityFilter = "Velocity Filter";
    const QString VisibleToEveryone = "Everyone";
    const QString VisibleToFriends = "Friends";
    const QString VisibleToNoOne = "No one";
    const QString Wireframe = "Wireframe";
}

#endif // hifi_Menu_h
