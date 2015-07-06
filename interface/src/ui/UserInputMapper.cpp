//
//  UserInputMapper.cpp
//  interface/src/ui
//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include <algorithm>

#include "Application.h"

#include "UserInputMapper.h"


// UserInputMapper Class

// Default contruct allocate the poutput size with the current hardcoded action channels
UserInputMapper::UserInputMapper() {
    assignDefaulActionScales();
    createActionNames();
}

bool UserInputMapper::registerDevice(uint16 deviceID, const DeviceProxy::Pointer& proxy){
    proxy->_name += " (" + QString::number(deviceID) + ")";
    _registeredDevices[deviceID] = proxy;
    return true;
}

UserInputMapper::DeviceProxy::Pointer UserInputMapper::getDeviceProxy(const Input& input) {
    auto device = _registeredDevices.find(input.getDevice());
    if (device != _registeredDevices.end()) {
        return (device->second);
    } else {
        return DeviceProxy::Pointer();
    }
}

void UserInputMapper::resetAllDeviceBindings() {
    for (auto device : _registeredDevices) {
        device.second->resetDeviceBindings();
    }
}

void UserInputMapper::resetDevice(uint16 deviceID) {
    auto device = _registeredDevices.find(deviceID);
    if (device != _registeredDevices.end()) {
        device->second->resetDeviceBindings();
    }
}

int UserInputMapper::findDevice(QString name) {
    for (auto device : _registeredDevices) {
        if (device.second->_name.split(" (")[0] == name) {
            return device.first;
        }
    }
    return 0;
}

bool UserInputMapper::addInputChannel(Action action, const Input& input, float scale) {
    return addInputChannel(action, input, Input(), scale);
}

bool UserInputMapper::addInputChannel(Action action, const Input& input, const Input& modifier, float scale) {
    // Check that the device is registered
    if (!getDeviceProxy(input)) {
        qDebug() << "UserInputMapper::addInputChannel: The input comes from a device #" << input.getDevice() << "is unknown. no inputChannel mapped.";
        return false;
    }
    
    auto inputChannel = InputChannel(input, modifier, action, scale);

    // Insert or replace the input to modifiers
    if (inputChannel.hasModifier()) {
        auto& modifiers = _inputToModifiersMap[input.getID()];
        modifiers.push_back(inputChannel._modifier);
        std::sort(modifiers.begin(), modifiers.end());
    }

    // Now update the action To Inputs side of things
    _actionToInputsMap.insert(ActionToInputsMap::value_type(action, inputChannel));

    return true;
}

int UserInputMapper::addInputChannels(const InputChannels& channels) {
    int nbAdded = 0;
    for (auto& channel : channels) {
        nbAdded += addInputChannel(channel._action, channel._input, channel._modifier, channel._scale);
    }
    return nbAdded;
}

bool UserInputMapper::removeInputChannel(InputChannel inputChannel) {
    // Remove from Input to Modifiers map
    if (inputChannel.hasModifier()) {
        _inputToModifiersMap.erase(inputChannel._input.getID());
    }
    
    // Remove from Action to Inputs map
    std::pair<ActionToInputsMap::iterator, ActionToInputsMap::iterator> ret;
    ret = _actionToInputsMap.equal_range(inputChannel._action);
    for (ActionToInputsMap::iterator it=ret.first; it!=ret.second; ++it) {
        if (it->second == inputChannel) {
            _actionToInputsMap.erase(it);
            return true;
        }
    }
    
    return false;
}

void UserInputMapper::removeAllInputChannels() {
    _inputToModifiersMap.clear();
    _actionToInputsMap.clear();
}

void UserInputMapper::removeAllInputChannelsForDevice(uint16 device) {
    QVector<InputChannel> channels = getAllInputsForDevice(device);
    for (auto& channel : channels) {
        removeInputChannel(channel);
    }
}

void UserInputMapper::removeDevice(int device) {
    removeAllInputChannelsForDevice((uint16) device);
    _registeredDevices.erase(device);
}

int UserInputMapper::getInputChannels(InputChannels& channels) const {
    for (auto& channel : _actionToInputsMap) {
        channels.push_back(channel.second);
    }

    return _actionToInputsMap.size();
}

QVector<UserInputMapper::InputChannel> UserInputMapper::getAllInputsForDevice(uint16 device) {
    InputChannels allChannels;
    getInputChannels(allChannels);
    
    QVector<InputChannel> channels;
    for (InputChannel inputChannel : allChannels) {
        if (inputChannel._input._device == device) {
            channels.push_back(inputChannel);
        }
    }
    
    return channels;
}

void UserInputMapper::update(float deltaTime) {

    // Reset the axis state for next loop
    for (auto& channel : _actionStates) {
        channel = 0.0f;
    }

    int currentTimestamp = 0;

    for (auto& channelInput : _actionToInputsMap) {
        auto& inputMapping = channelInput.second;
        auto& inputID = inputMapping._input;
        bool enabled = true;
        
        // Check if this input channel has modifiers and collect the possibilities
        auto modifiersIt = _inputToModifiersMap.find(inputID.getID());
        if (modifiersIt != _inputToModifiersMap.end()) {
            Modifiers validModifiers;
            bool isActiveModifier = false;
            for (auto& modifier : modifiersIt->second) {
                auto deviceProxy = getDeviceProxy(modifier);
                if (deviceProxy->getButton(modifier, currentTimestamp)) {
                    validModifiers.push_back(modifier);
                    isActiveModifier |= (modifier.getID() == inputMapping._modifier.getID());
                }
            }
            enabled = (validModifiers.empty() && !inputMapping.hasModifier()) || isActiveModifier;
        }

        // if enabled: default input or all modifiers on
        if (enabled) {
            auto deviceProxy = getDeviceProxy(inputID);
            switch (inputMapping._input.getType()) {
                case ChannelType::BUTTON: {
                    _actionStates[channelInput.first] += inputMapping._scale * float(deviceProxy->getButton(inputID, currentTimestamp));// * deltaTime; // weight the impulse by the deltaTime
                    break;
                }
                case ChannelType::AXIS: {
                    _actionStates[channelInput.first] += inputMapping._scale * deviceProxy->getAxis(inputID, currentTimestamp);
                    break;
                }
                case ChannelType::JOINT: {
                    // _channelStates[channelInput.first].jointVal = deviceProxy->getJoint(inputID, currentTimestamp);
                    break;
                }
                default: {
                    break; //silence please
                }
            }
        } else{
            // Channel input not enabled
            enabled = false;
        }
    }

    // Scale all the channel step with the scale
    for (auto i = 0; i < NUM_ACTIONS; i++) {
        _actionStates[i] *= _actionScales[i];
        if (_actionStates[i] > 0) {
            emit Application::getInstance()->getControllerScriptingInterface()->actionEvent(i, _actionStates[i]);
        }
    }
}

QVector<UserInputMapper::Action> UserInputMapper::getAllActions() {
    QVector<Action> actions;
    for (auto i = 0; i < NUM_ACTIONS; i++) {
        actions.append(Action(i));
    }
    return actions;
}

QVector<UserInputMapper::InputChannel> UserInputMapper::getInputChannelsForAction(UserInputMapper::Action action) {
    QVector<InputChannel> inputChannels;
    std::pair <ActionToInputsMap::iterator, ActionToInputsMap::iterator> ret;
    ret = _actionToInputsMap.equal_range(action);
    for (ActionToInputsMap::iterator it=ret.first; it!=ret.second; ++it) {
        inputChannels.append(it->second);
    }
    return inputChannels;
}

void UserInputMapper::assignDefaulActionScales() {
    _actionScales[LONGITUDINAL_BACKWARD] = 1.0f; // 1m per unit
    _actionScales[LONGITUDINAL_FORWARD] = 1.0f; // 1m per unit
    _actionScales[LATERAL_LEFT] = 1.0f; // 1m per unit
    _actionScales[LATERAL_RIGHT] = 1.0f; // 1m per unit
    _actionScales[VERTICAL_DOWN] = 1.0f; // 1m per unit
    _actionScales[VERTICAL_UP] = 1.0f; // 1m per unit
    _actionScales[YAW_LEFT] = 1.0f; // 1 degree per unit
    _actionScales[YAW_RIGHT] = 1.0f; // 1 degree per unit
    _actionScales[PITCH_DOWN] = 1.0f; // 1 degree per unit
    _actionScales[PITCH_UP] = 1.0f; // 1 degree per unit
    _actionScales[BOOM_IN] = 0.5f; // .5m per unit
    _actionScales[BOOM_OUT] = 0.5f; // .5m per unit
    _actionStates[SHIFT] = 1.0f; // on
    _actionStates[ACTION1] = 1.0f; // default
    _actionStates[ACTION2] = 1.0f; // default
}

// This is only necessary as long as the actions are hardcoded
// Eventually you can just add the string when you add the action
void UserInputMapper::createActionNames() {
    _actionNames[LONGITUDINAL_BACKWARD] = "LONGITUDINAL_BACKWARD";
    _actionNames[LONGITUDINAL_FORWARD] = "LONGITUDINAL_FORWARD";
    _actionNames[LATERAL_LEFT] = "LATERAL_LEFT";
    _actionNames[LATERAL_RIGHT] = "LATERAL_RIGHT";
    _actionNames[VERTICAL_DOWN] = "VERTICAL_DOWN";
    _actionNames[VERTICAL_UP] = "VERTICAL_UP";
    _actionNames[YAW_LEFT] = "YAW_LEFT";
    _actionNames[YAW_RIGHT] = "YAW_RIGHT";
    _actionNames[PITCH_DOWN] = "PITCH_DOWN";
    _actionNames[PITCH_UP] = "PITCH_UP";
    _actionNames[BOOM_IN] = "BOOM_IN";
    _actionNames[BOOM_OUT] = "BOOM_OUT";
    _actionNames[SHIFT] = "SHIFT";
    _actionNames[ACTION1] = "ACTION1";
    _actionNames[ACTION2] = "ACTION2";
}