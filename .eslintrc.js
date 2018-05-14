module.exports = {
    "root": true,
    "extends": "eslint:recommended",
    "parserOptions": {
        "ecmaVersion": 5
    },
    "globals": {
        "Account": false,
        "Agent": false,
        "AnimationCache": false,
        "Assets": false,
        "Audio": false,
        "AudioDevice": false,
        "AudioEffectOptions": false,
        "Avatar": false,
        "AvatarList": false,
        "AvatarManager": false,
        "Camera": false,
        "Clipboard": false,
        "console": false,
        "ContextOverlay": false,
        "Controller": false,
        "DebugDraw": false,
        "DialogsManager": false,
        "Entities": false,
        "EntityViewer": false,
        "FaceTracker": false,
        "GlobalServices": false,
        "HMD": false,
        "LaserPointers": false,
        "location": true,
        "LODManager": false,
        "Mat4": false,
        "Menu": false,
        "Messages": false,
        "ModelCache": false,
        "module": false,
        "MyAvatar": false,
        "Overlays": false,
        "OverlayWebWindow": false,
        "Paths": false,
        "print": false,
        "Quat": false,
        "Rates": false,
        "RayPick": false,
        "Recording": false,
        "Render": false,
        "Resource": false,
        "Reticle": false,
        "Scene": false,
        "Script": false,
        "ScriptDiscoveryService": false,
        "Settings": false,
        "SoundCache": false,
        "Stats": false,
        "Tablet": false,
        "TextureCache": false,
        "Toolbars": false,
        "UndoStack": false,
        "Users": false,
        "UserActivityLogger": false,
        "Uuid": false,
        "Vec3": false,
        "WebSocket": false,
        "WebWindow": false,
        "Window": false,
        "XMLHttpRequest": false
    },
    "rules": {
        "brace-style": ["error", "1tbs", {"allowSingleLine": false}],
        "camelcase": ["error"],
        "comma-dangle": ["error", "never"],
        "curly": ["error", "all"],
        "eqeqeq": ["error", "always"],
        "indent": ["error", 4, {"SwitchCase": 1}],
        "key-spacing": ["error", {"beforeColon": false, "afterColon": true, "mode": "strict"}],
        "keyword-spacing": ["error", {"before": true, "after": true}],
        "max-len": ["error", 128, 4],
        "new-cap": ["error"],
        "no-console": ["off"],
        "no-floating-decimal": ["error"],
        "no-magic-numbers": ["error", {"ignore": [-1, 0, 1], "ignoreArrayIndexes": true}],
        "no-multi-spaces": ["error"],
        "no-multiple-empty-lines": ["error"],
        "no-unused-vars": ["error", {"args": "none", "vars": "local"}],
        "semi": ["error", "always"],
        "space-before-blocks": ["error"],
        "space-before-function-paren": ["error", {"anonymous": "ignore", "named": "never"}],
        "spaced-comment": ["error", "always", {"line": {"markers": ["/"]}}]
    }
};
