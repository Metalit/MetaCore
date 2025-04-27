#pragma once

#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/Pose.hpp"
#include "VRUIControls/VRInputModule.hpp"

namespace MetaCore::Input {
    enum Buttons {
        // The lower buttons on the controller's face, A or X
        AX,
        // The upper buttons on the controller's face, B or Y
        BY,
        // Specficially pressing on the thumbstick
        Thumbstick,
        // The trigger on the grip of the controller
        Grip,
        // The trigger on the front of the controller
        Trigger,
        // The button normally used to pause a map, only on the left controller
        Start,
        ButtonsMax = Start,
    };

    enum Axes {
        // The vertical component of the thumbstick position
        ThumbstickVertical,
        // The horizontal component of the thumbstick position
        ThumbstickHorizontal,
        // The amount the trigger on the grip of the controller is pressed
        GripStrength,
        // The amount the trigger on the front of the controller is pressed
        TriggerStrength,
    };

    enum Controllers {
        // The left hand controller
        Left,
        // The right hand controller
        Right,
        // The controller with the most recently pressed button
        Active,
        // Either controller
        Either,
    };

    /// @brief Gets if a button is currently pressed for a given controller
    /// @param controller The controller to check the button state for
    /// @param button The button to check
    /// @return If the button is pressed on the controller
    bool GetPressed(Controllers controller, Buttons button);
    /// @brief Gets a 1-dimensional axis or strength of an input
    /// @param controller The controller to check the axis state for. If Either, the maximum will be returned
    /// @param axis The axis input to check
    /// @return The axis amount from 0 to 1, or -1 to 1 if a thumbstick axis
    float GetAxis(Controllers controller, Axes axis);
    /// @brief Gets the current thumbstick position of a controller
    /// @param controller The controller to check the thumbstick state for. If Either, the maximum will be returned
    /// @return The 2-dimensional thumbstick position with axes from -1 to 1
    UnityEngine::Vector2 GetThumbstick(Controllers controller);

    /// @brief The string identifier that maps the Buttons enum to events, when used in the "mod" parameter
    std::string const ButtonEvents = "MetaCoreButtons";

    /// @brief Gets the current position and rotation of a controller
    /// @param left If the left controller should be returned, otherwise the right controller
    /// @return The tracked pose of the controller
    UnityEngine::Pose GetHandPose(bool left);
    /// @brief Gets the current position and rotation of the headset
    /// @return The tracked pose of the headset
    UnityEngine::Pose GetHeadPose();

    /// @brief Finds the currently active VRInputModule if any
    /// @return The currently active VRInputModule, or nullptr if none can be found
    VRUIControls::VRInputModule* GetCurrentInputModule();

    /// @brief Enables or disables haptic feedback for a mod. Haptics will only be enabled if no mods have it disabled
    /// @param mod The unique id of the mod modifying haptic feedback
    /// @param enable If haptic feedback should be enabled for this mod
    void SetHaptics(std::string mod, bool enable);
}
