
#include "EntityPropertyFlags.h"


QString EntityPropertyFlagsToString(EntityPropertyFlags propertiesFlags) {
    QString result = QString("[ ");
    if (propertiesFlags.getHasProperty(PROP_PAGED_PROPERTY)) {
        result += "paged_property ";
    }
    if (propertiesFlags.getHasProperty(PROP_CUSTOM_PROPERTIES_INCLUDED)) {
        result += "custom_properties_included ";
    }
    if (propertiesFlags.getHasProperty(PROP_VISIBLE)) {
        result += "visible ";
    }
    if (propertiesFlags.getHasProperty(PROP_CAN_CAST_SHADOW)) {
        result += "can_cast_shadow ";
    }
    if (propertiesFlags.getHasProperty(PROP_POSITION)) {
        result += "position ";
    }
    if (propertiesFlags.getHasProperty(PROP_DIMENSIONS)) {
        result += "dimensions ";
    }
    if (propertiesFlags.getHasProperty(PROP_ROTATION)) {
        result += "rotation ";
    }
    if (propertiesFlags.getHasProperty(PROP_DENSITY)) {
        result += "density ";
    }
    if (propertiesFlags.getHasProperty(PROP_VELOCITY)) {
        result += "velocity ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAVITY)) {
        result += "gravity ";
    }
    if (propertiesFlags.getHasProperty(PROP_DAMPING)) {
        result += "damping ";
    }
    if (propertiesFlags.getHasProperty(PROP_LIFETIME)) {
        result += "lifetime ";
    }
    if (propertiesFlags.getHasProperty(PROP_SCRIPT)) {
        result += "script ";
    }
    if (propertiesFlags.getHasProperty(PROP_COLOR)) {
        result += "color ";
    }
    if (propertiesFlags.getHasProperty(PROP_MODEL_URL)) {
        result += "model_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_URL)) {
        result += "animation_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_FPS)) {
        result += "animation_fps ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_FRAME_INDEX)) {
        result += "animation_frame_index ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_PLAYING)) {
        result += "animation_playing ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_ALLOW_TRANSLATION)) {
        result += "animation_allow_translation ";
    }
    if (propertiesFlags.getHasProperty(PROP_RELAY_PARENT_JOINTS)) {
        result += "relay_parent_joints ";
    }
    if (propertiesFlags.getHasProperty(PROP_REGISTRATION_POINT)) {
        result += "registration_point ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANGULAR_VELOCITY)) {
        result += "angular_velocity ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANGULAR_DAMPING)) {
        result += "angular_damping ";
    }
    if (propertiesFlags.getHasProperty(PROP_COLLISIONLESS)) {
        result += "collisionless ";
    }
    if (propertiesFlags.getHasProperty(PROP_DYNAMIC)) {
        result += "dynamic ";
    }
    if (propertiesFlags.getHasProperty(PROP_IS_SPOTLIGHT)) {
        result += "is_spotlight ";
    }
    if (propertiesFlags.getHasProperty(PROP_DIFFUSE_COLOR)) {
        result += "diffuse_color ";
    }
    if (propertiesFlags.getHasProperty(PROP_AMBIENT_COLOR_UNUSED)) {
        result += "ambient_color_unused ";
    }
    if (propertiesFlags.getHasProperty(PROP_SPECULAR_COLOR_UNUSED)) {
        result += "specular_color_unused ";
    }
    if (propertiesFlags.getHasProperty(PROP_INTENSITY)) {
        result += "intensity ";
    }
    if (propertiesFlags.getHasProperty(PROP_LINEAR_ATTENUATION_UNUSED)) {
        result += "linear_attenuation_unused ";
    }
    if (propertiesFlags.getHasProperty(PROP_QUADRATIC_ATTENUATION_UNUSED)) {
        result += "quadratic_attenuation_unused ";
    }
    if (propertiesFlags.getHasProperty(PROP_EXPONENT)) {
        result += "exponent ";
    }
    if (propertiesFlags.getHasProperty(PROP_CUTOFF)) {
        result += "cutoff ";
    }
    if (propertiesFlags.getHasProperty(PROP_LOCKED)) {
        result += "locked ";
    }
    if (propertiesFlags.getHasProperty(PROP_TEXTURES)) {
        result += "textures ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_SETTINGS_UNUSED)) {
        result += "animation_settings_unused ";
    }
    if (propertiesFlags.getHasProperty(PROP_USER_DATA)) {
        result += "user_data ";
    }
    if (propertiesFlags.getHasProperty(PROP_SHAPE_TYPE)) {
        result += "shape_type ";
    }
    if (propertiesFlags.getHasProperty(PROP_MAX_PARTICLES)) {
        result += "max_particles ";
    }
    if (propertiesFlags.getHasProperty(PROP_LIFESPAN)) {
        result += "lifespan ";
    }
    if (propertiesFlags.getHasProperty(PROP_EMIT_RATE)) {
        result += "emit_rate ";
    }
    if (propertiesFlags.getHasProperty(PROP_EMIT_SPEED)) {
        result += "emit_speed ";
    }
    if (propertiesFlags.getHasProperty(PROP_EMIT_STRENGTH)) {
        result += "emit_strength ";
    }
    if (propertiesFlags.getHasProperty(PROP_EMIT_ACCELERATION)) {
        result += "emit_acceleration ";
    }
    if (propertiesFlags.getHasProperty(PROP_PARTICLE_RADIUS)) {
        result += "particle_radius ";
    }
    if (propertiesFlags.getHasProperty(PROP_COMPOUND_SHAPE_URL)) {
        result += "compound_shape_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_MARKETPLACE_ID)) {
        result += "marketplace_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_ACCELERATION)) {
        result += "acceleration ";
    }
    if (propertiesFlags.getHasProperty(PROP_SIMULATION_OWNER)) {
        result += "simulation_owner ";
    }
    if (propertiesFlags.getHasProperty(PROP_NAME)) {
        result += "name ";
    }
    if (propertiesFlags.getHasProperty(PROP_COLLISION_SOUND_URL)) {
        result += "collision_sound_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_RESTITUTION)) {
        result += "restitution ";
    }
    if (propertiesFlags.getHasProperty(PROP_FRICTION)) {
        result += "friction ";
    }
    if (propertiesFlags.getHasProperty(PROP_VOXEL_VOLUME_SIZE)) {
        result += "voxel_volume_size ";
    }
    if (propertiesFlags.getHasProperty(PROP_VOXEL_DATA)) {
        result += "voxel_data ";
    }
    if (propertiesFlags.getHasProperty(PROP_VOXEL_SURFACE_STYLE)) {
        result += "voxel_surface_style ";
    }
    if (propertiesFlags.getHasProperty(PROP_LINE_WIDTH)) {
        result += "line_width ";
    }
    if (propertiesFlags.getHasProperty(PROP_LINE_POINTS)) {
        result += "line_points ";
    }
    if (propertiesFlags.getHasProperty(PROP_HREF)) {
        result += "href ";
    }
    if (propertiesFlags.getHasProperty(PROP_DESCRIPTION)) {
        result += "description ";
    }
    if (propertiesFlags.getHasProperty(PROP_FACE_CAMERA)) {
        result += "face_camera ";
    }
    if (propertiesFlags.getHasProperty(PROP_SCRIPT_TIMESTAMP)) {
        result += "script_timestamp ";
    }
    if (propertiesFlags.getHasProperty(PROP_ACTION_DATA)) {
        result += "action_data ";
    }
    if (propertiesFlags.getHasProperty(PROP_X_TEXTURE_URL)) {
        result += "x_texture_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_Y_TEXTURE_URL)) {
        result += "y_texture_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_Z_TEXTURE_URL)) {
        result += "z_texture_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_NORMALS)) {
        result += "normals ";
    }
    if (propertiesFlags.getHasProperty(PROP_STROKE_COLORS)) {
        result += "stroke_colors ";
    }
    if (propertiesFlags.getHasProperty(PROP_STROKE_WIDTHS)) {
        result += "stroke_widths ";
    }
    if (propertiesFlags.getHasProperty(PROP_IS_UV_MODE_STRETCH)) {
        result += "is_uv_mode_stretch ";
    }
    if (propertiesFlags.getHasProperty(PROP_SPEED_SPREAD)) {
        result += "speed_spread ";
    }
    if (propertiesFlags.getHasProperty(PROP_ACCELERATION_SPREAD)) {
        result += "acceleration_spread ";
    }
    if (propertiesFlags.getHasProperty(PROP_X_N_NEIGHBOR_ID)) {
        result += "x_n_neighbor_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_Y_N_NEIGHBOR_ID)) {
        result += "y_n_neighbor_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_Z_N_NEIGHBOR_ID)) {
        result += "z_n_neighbor_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_X_P_NEIGHBOR_ID)) {
        result += "x_p_neighbor_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_Y_P_NEIGHBOR_ID)) {
        result += "y_p_neighbor_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_Z_P_NEIGHBOR_ID)) {
        result += "z_p_neighbor_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_RADIUS_SPREAD)) {
        result += "radius_spread ";
    }
    if (propertiesFlags.getHasProperty(PROP_RADIUS_START)) {
        result += "radius_start ";
    }
    if (propertiesFlags.getHasProperty(PROP_RADIUS_FINISH)) {
        result += "radius_finish ";
    }
    if (propertiesFlags.getHasProperty(PROP_ALPHA)) {
        result += "alpha ";
    }
    if (propertiesFlags.getHasProperty(PROP_COLOR_SPREAD)) {
        result += "color_spread ";
    }
    if (propertiesFlags.getHasProperty(PROP_COLOR_START)) {
        result += "color_start ";
    }
    if (propertiesFlags.getHasProperty(PROP_COLOR_FINISH)) {
        result += "color_finish ";
    }
    if (propertiesFlags.getHasProperty(PROP_ALPHA_SPREAD)) {
        result += "alpha_spread ";
    }
    if (propertiesFlags.getHasProperty(PROP_ALPHA_START)) {
        result += "alpha_start ";
    }
    if (propertiesFlags.getHasProperty(PROP_ALPHA_FINISH)) {
        result += "alpha_finish ";
    }
    if (propertiesFlags.getHasProperty(PROP_EMIT_ORIENTATION)) {
        result += "emit_orientation ";
    }
    if (propertiesFlags.getHasProperty(PROP_EMIT_DIMENSIONS)) {
        result += "emit_dimensions ";
    }
    if (propertiesFlags.getHasProperty(PROP_EMIT_RADIUS_START)) {
        result += "emit_radius_start ";
    }
    if (propertiesFlags.getHasProperty(PROP_POLAR_START)) {
        result += "polar_start ";
    }
    if (propertiesFlags.getHasProperty(PROP_POLAR_FINISH)) {
        result += "polar_finish ";
    }
    if (propertiesFlags.getHasProperty(PROP_AZIMUTH_START)) {
        result += "azimuth_start ";
    }
    if (propertiesFlags.getHasProperty(PROP_AZIMUTH_FINISH)) {
        result += "azimuth_finish ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_LOOP)) {
        result += "animation_loop ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_FIRST_FRAME)) {
        result += "animation_first_frame ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_LAST_FRAME)) {
        result += "animation_last_frame ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_HOLD)) {
        result += "animation_hold ";
    }
    if (propertiesFlags.getHasProperty(PROP_ANIMATION_START_AUTOMATICALLY)) {
        result += "animation_start_automatically ";
    }
    if (propertiesFlags.getHasProperty(PROP_EMITTER_SHOULD_TRAIL)) {
        result += "emitter_should_trail ";
    }
    if (propertiesFlags.getHasProperty(PROP_PARENT_ID)) {
        result += "parent_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_PARENT_JOINT_INDEX)) {
        result += "parent_joint_index ";
    }
    if (propertiesFlags.getHasProperty(PROP_LOCAL_POSITION)) {
        result += "local_position ";
    }
    if (propertiesFlags.getHasProperty(PROP_LOCAL_ROTATION)) {
        result += "local_rotation ";
    }
    if (propertiesFlags.getHasProperty(PROP_QUERY_AA_CUBE)) {
        result += "query_aa_cube ";
    }
    if (propertiesFlags.getHasProperty(PROP_JOINT_ROTATIONS_SET)) {
        result += "joint_rotations_set ";
    }
    if (propertiesFlags.getHasProperty(PROP_JOINT_ROTATIONS)) {
        result += "joint_rotations ";
    }
    if (propertiesFlags.getHasProperty(PROP_JOINT_TRANSLATIONS_SET)) {
        result += "joint_translations_set ";
    }
    if (propertiesFlags.getHasProperty(PROP_JOINT_TRANSLATIONS)) {
        result += "joint_translations ";
    }
    if (propertiesFlags.getHasProperty(PROP_COLLISION_MASK)) {
        result += "collision_mask ";
    }
    if (propertiesFlags.getHasProperty(PROP_FALLOFF_RADIUS)) {
        result += "falloff_radius ";
    }
    if (propertiesFlags.getHasProperty(PROP_FLYING_ALLOWED)) {
        result += "flying_allowed ";
    }
    if (propertiesFlags.getHasProperty(PROP_GHOSTING_ALLOWED)) {
        result += "ghosting_allowed ";
    }
    if (propertiesFlags.getHasProperty(PROP_CLIENT_ONLY)) {
        result += "client_only ";
    }
    if (propertiesFlags.getHasProperty(PROP_OWNING_AVATAR_ID)) {
        result += "owning_avatar_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_SHAPE)) {
        result += "shape ";
    }
    if (propertiesFlags.getHasProperty(PROP_DPI)) {
        result += "dpi ";
    }
    if (propertiesFlags.getHasProperty(PROP_LOCAL_VELOCITY)) {
        result += "local_velocity ";
    }
    if (propertiesFlags.getHasProperty(PROP_LOCAL_ANGULAR_VELOCITY)) {
        result += "local_angular_velocity ";
    }
    if (propertiesFlags.getHasProperty(PROP_LAST_EDITED_BY)) {
        result += "last_edited_by ";
    }
    if (propertiesFlags.getHasProperty(PROP_SERVER_SCRIPTS)) {
        result += "server_scripts ";
    }
    if (propertiesFlags.getHasProperty(PROP_FILTER_URL)) {
        result += "filter_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_ITEM_NAME)) {
        result += "item_name ";
    }
    if (propertiesFlags.getHasProperty(PROP_ITEM_DESCRIPTION)) {
        result += "item_description ";
    }
    if (propertiesFlags.getHasProperty(PROP_ITEM_CATEGORIES)) {
        result += "item_categories ";
    }
    if (propertiesFlags.getHasProperty(PROP_ITEM_ARTIST)) {
        result += "item_artist ";
    }
    if (propertiesFlags.getHasProperty(PROP_ITEM_LICENSE)) {
        result += "item_license ";
    }
    if (propertiesFlags.getHasProperty(PROP_LIMITED_RUN)) {
        result += "limited_run ";
    }
    if (propertiesFlags.getHasProperty(PROP_EDITION_NUMBER)) {
        result += "edition_number ";
    }
    if (propertiesFlags.getHasProperty(PROP_ENTITY_INSTANCE_NUMBER)) {
        result += "entity_instance_number ";
    }
    if (propertiesFlags.getHasProperty(PROP_CERTIFICATE_ID)) {
        result += "certificate_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_STATIC_CERTIFICATE_VERSION)) {
        result += "static_certificate_version ";
    }
    if (propertiesFlags.getHasProperty(PROP_CLONEABLE)) {
        result += "cloneable ";
    }
    if (propertiesFlags.getHasProperty(PROP_CLONE_LIFETIME)) {
        result += "clone_lifetime ";
    }
    if (propertiesFlags.getHasProperty(PROP_CLONE_LIMIT)) {
        result += "clone_limit ";
    }
    if (propertiesFlags.getHasProperty(PROP_CLONE_DYNAMIC)) {
        result += "clone_dynamic ";
    }
    if (propertiesFlags.getHasProperty(PROP_CLONE_AVATAR_ENTITY)) {
        result += "clone_avatar_entity ";
    }
    if (propertiesFlags.getHasProperty(PROP_CLONE_ORIGIN_ID)) {
        result += "clone_origin_id ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_MODE)) {
        result += "haze_mode ";
    }
    if (propertiesFlags.getHasProperty(PROP_KEYLIGHT_COLOR)) {
        result += "keylight_color ";
    }
    if (propertiesFlags.getHasProperty(PROP_KEYLIGHT_INTENSITY)) {
        result += "keylight_intensity ";
    }
    if (propertiesFlags.getHasProperty(PROP_KEYLIGHT_DIRECTION)) {
        result += "keylight_direction ";
    }
    if (propertiesFlags.getHasProperty(PROP_KEYLIGHT_CAST_SHADOW)) {
        result += "keylight_cast_shadow ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_RANGE)) {
        result += "haze_range ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_COLOR)) {
        result += "haze_color ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_GLARE_COLOR)) {
        result += "haze_glare_color ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_ENABLE_GLARE)) {
        result += "haze_enable_glare ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_GLARE_ANGLE)) {
        result += "haze_glare_angle ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_ALTITUDE_EFFECT)) {
        result += "haze_altitude_effect ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_CEILING)) {
        result += "haze_ceiling ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_BASE_REF)) {
        result += "haze_base_ref ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_BACKGROUND_BLEND)) {
        result += "haze_background_blend ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_ATTENUATE_KEYLIGHT)) {
        result += "haze_attenuate_keylight ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_KEYLIGHT_RANGE)) {
        result += "haze_keylight_range ";
    }
    if (propertiesFlags.getHasProperty(PROP_HAZE_KEYLIGHT_ALTITUDE)) {
        result += "haze_keylight_altitude ";
    }
    if (propertiesFlags.getHasProperty(PROP_KEY_LIGHT_MODE)) {
        result += "key_light_mode ";
    }
    if (propertiesFlags.getHasProperty(PROP_AMBIENT_LIGHT_MODE)) {
        result += "ambient_light_mode ";
    }
    if (propertiesFlags.getHasProperty(PROP_SKYBOX_MODE)) {
        result += "skybox_mode ";
    }
    if (propertiesFlags.getHasProperty(PROP_LOCAL_DIMENSIONS)) {
        result += "local_dimensions ";
    }
    if (propertiesFlags.getHasProperty(PROP_MATERIAL_URL)) {
        result += "material_url ";
    }
    if (propertiesFlags.getHasProperty(PROP_MATERIAL_MAPPING_MODE)) {
        result += "material_mapping_mode ";
    }
    if (propertiesFlags.getHasProperty(PROP_MATERIAL_PRIORITY)) {
        result += "material_priority ";
    }
    if (propertiesFlags.getHasProperty(PROP_PARENT_MATERIAL_NAME)) {
        result += "parent_material_name ";
    }
    if (propertiesFlags.getHasProperty(PROP_MATERIAL_MAPPING_POS)) {
        result += "material_mapping_pos ";
    }
    if (propertiesFlags.getHasProperty(PROP_MATERIAL_MAPPING_SCALE)) {
        result += "material_mapping_scale ";
    }
    if (propertiesFlags.getHasProperty(PROP_MATERIAL_MAPPING_ROT)) {
        result += "material_mapping_rot ";
    }
    if (propertiesFlags.getHasProperty(PROP_MATERIAL_DATA)) {
        result += "material_data ";
    }
    if (propertiesFlags.getHasProperty(PROP_VISIBLE_IN_SECONDARY_CAMERA)) {
        result += "visible_in_secondary_camera ";
    }
    if (propertiesFlags.getHasProperty(PROP_PARTICLE_SPIN)) {
        result += "particle_spin ";
    }
    if (propertiesFlags.getHasProperty(PROP_SPIN_START)) {
        result += "spin_start ";
    }
    if (propertiesFlags.getHasProperty(PROP_SPIN_FINISH)) {
        result += "spin_finish ";
    }
    if (propertiesFlags.getHasProperty(PROP_SPIN_SPREAD)) {
        result += "spin_spread ";
    }
    if (propertiesFlags.getHasProperty(PROP_PARTICLE_ROTATE_WITH_ENTITY)) {
        result += "particle_rotate_with_entity ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_GRABBABLE)) {
        result += "grab_grabbable ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_KINEMATIC)) {
        result += "grab_kinematic ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_FOLLOWS_CONTROLLER)) {
        result += "grab_follows_controller ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_TRIGGERABLE)) {
        result += "grab_triggerable ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_EQUIPPABLE)) {
        result += "grab_equippable ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET)) {
        result += "grab_left_equippable_position_offset ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET)) {
        result += "grab_left_equippable_rotation_offset ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET)) {
        result += "grab_right_equippable_position_offset ";
    }
    if (propertiesFlags.getHasProperty(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET)) {
        result += "grab_right_equippable_rotation_offset ";
    }

    result += QString("]");

    return result;
}
