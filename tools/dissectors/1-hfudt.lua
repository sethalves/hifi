print("Loading hfudt")

-- create the HFUDT protocol
p_hfudt = Proto("hfudt", "HFUDT Protocol")

-- create fields shared between packets in HFUDT
local f_data = ProtoField.string("hfudt.data", "Data")

-- create the fields for data packets in HFUDT
local f_length = ProtoField.uint16("hfudt.length", "Length", base.DEC)
local f_control_bit = ProtoField.uint8("hfudt.control", "Control Bit", base.DEC)
local f_reliable_bit = ProtoField.uint8("hfudt.reliable", "Reliability Bit", base.DEC)
local f_message_bit = ProtoField.uint8("hfudt.message", "Message Bit", base.DEC)
local f_obfuscation_level = ProtoField.uint8("hfudt.obfuscation_level", "Obfuscation Level", base.DEC)
local f_sequence_number = ProtoField.uint32("hfudt.sequence_number", "Sequence Number", base.DEC)
local f_message_position = ProtoField.uint8("hfudt.message_position", "Message Position", base.DEC)
local f_message_number = ProtoField.uint32("hfudt.message_number", "Message Number", base.DEC)
local f_message_part_number = ProtoField.uint32("hfudt.message_part_number", "Message Part Number", base.DEC)
local f_type = ProtoField.uint8("hfudt.type", "Type", base.DEC)
local f_version = ProtoField.uint8("hfudt.version", "Version", base.DEC)
local f_type_text = ProtoField.string("hfudt.type_text", "TypeText")
local f_sender_id = ProtoField.uint16("hfudt.sender_id", "Sender ID", base.DEC)
local f_hmac_hash = ProtoField.bytes("hfudt.hmac_hash", "HMAC Hash")

-- create the fields for control packets in HFUDT
local f_control_type = ProtoField.uint16("hfudt.control_type", "Control Type", base.DEC)
local f_control_type_text = ProtoField.string("hfudt.control_type_text", "Control Type Text", base.ASCII)
local f_ack_sequence_number = ProtoField.uint32("hfudt.ack_sequence_number", "ACKed Sequence Number", base.DEC)
local f_control_sub_sequence = ProtoField.uint32("hfudt.control_sub_sequence", "Control Sub-Sequence Number", base.DEC)
local f_nak_sequence_number = ProtoField.uint32("hfudt.nak_sequence_number", "NAKed Sequence Number", base.DEC)
local f_nak_range_end = ProtoField.uint32("hfudt.nak_range_end", "NAK Range End", base.DEC)

local SEQUENCE_NUMBER_MASK = 0x07FFFFFF

p_hfudt.fields = {
  f_length,
  f_control_bit, f_reliable_bit, f_message_bit, f_sequence_number, f_type, f_type_text, f_version,
  f_sender_id, f_hmac_hash,
  f_message_position, f_message_number, f_message_part_number, f_obfuscation_level,
  f_control_type, f_control_type_text, f_control_sub_sequence, f_ack_sequence_number, f_nak_sequence_number, f_nak_range_end,
  f_data
}

local control_types = {
  [0] = { "ACK", "Acknowledgement" },
  [1] = { "ACK2", "Acknowledgement of acknowledgement" },
  [2] = { "LightACK", "Light Acknowledgement" },
  [3] = { "NAK", "Loss report (NAK)" },
  [4] = { "TimeoutNAK", "Loss report re-transmission (TimeoutNAK)" },
  [5] = { "Handshake", "Handshake" },
  [6] = { "HandshakeACK", "Acknowledgement of Handshake" },
  [7] = { "ProbeTail", "Probe tail" },
  [8] = { "HandshakeRequest", "Request a Handshake" }
}

local message_positions = {
  [0] = "ONLY",
  [1] = "LAST",
  [2] = "FIRST",
  [3] = "MIDDLE"
}

local packet_types = {
  [0] = "Unknown",
  [1] = "StunResponse",
  [2] = "DomainList",
  [3] = "Ping",
  [4] = "PingReply",
  [5] = "KillAvatar",
  [6] = "AvatarData",
  [7] = "InjectAudio",
  [8] = "MixedAudio",
  [9] = "MicrophoneAudioNoEcho",
  [10] = "MicrophoneAudioWithEcho",
  [11] = "BulkAvatarData",
  [12] = "SilentAudioFrame",
  [13] = "DomainListRequest",
  [14] = "RequestAssignment",
  [15] = "CreateAssignment",
  [16] = "DomainConnectionDenied",
  [17] = "MuteEnvironment",
  [18] = "AudioStreamStats",
  [19] = "DomainServerPathQuery",
  [20] = "DomainServerPathResponse",
  [21] = "DomainServerAddedNode",
  [22] = "ICEServerPeerInformation",
  [23] = "ICEServerQuery",
  [24] = "OctreeStats",
  [25] = "Jurisdiction",
  [26] = "AvatarIdentityRequest",
  [27] = "AssignmentClientStatus",
  [28] = "NoisyMute",
  [29] = "AvatarIdentity",
  [30] = "AvatarBillboard",
  [31] = "DomainConnectRequest",
  [32] = "DomainServerRequireDTLS",
  [33] = "NodeJsonStats",
  [34] = "OctreeDataNack",
  [35] = "StopNode",
  [36] = "AudioEnvironment",
  [37] = "EntityEditNack",
  [38] = "ICEServerHeartbeat",
  [39] = "ICEPing",
  [40] = "ICEPingReply",
  [41] = "EntityData",
  [42] = "EntityQuery",
  [43] = "EntityAdd",
  [44] = "EntityErase",
  [45] = "EntityEdit",
  [46] = "DomainServerConnectionToken",
  [47] = "DomainSettingsRequest",
  [48] = "DomainSettings",
  [49] = "AssetGet",
  [50] = "AssetGetReply",
  [51] = "AssetUpload",
  [52] = "AssetUploadReply",
  [53] = "AssetGetInfo",
  [54] = "AssetGetInfoReply"
}

local unsourced_packet_types = {
  ["DomainList"] = true
}

function p_hfudt.dissector(buf, pinfo, tree)

   -- make sure this isn't a STUN packet - those don't follow HFUDT format
  if pinfo.dst == Address.ip("stun.highfidelity.io") then return end

  -- validate that the packet length is at least the minimum control packet size
  if buf:len() < 4 then return end

  -- create a subtree for HFUDT
  subtree = tree:add(p_hfudt, buf(0))

  -- set the packet length
  subtree:add(f_length, buf:len())

  -- pull out the entire first word
  local first_word = buf(0, 4):le_uint()

  -- pull out the control bit and add it to the subtree
  local control_bit = bit32.rshift(first_word, 31)
  subtree:add(f_control_bit, control_bit)

  local data_length = 0

  if control_bit == 1 then
    -- dissect the control packet
    pinfo.cols.protocol = p_hfudt.name .. " Control"

    -- remove the control bit and shift to the right to get the type value
    local shifted_type = bit32.rshift(bit32.lshift(first_word, 1), 17)
    local type = subtree:add(f_control_type, shifted_type)

    if control_types[shifted_type] ~= nil then
      -- if we know this type then add the name
      type:append_text(" (".. control_types[shifted_type][1] .. ")")

      subtree:add(f_control_type_text, control_types[shifted_type][1])
    end

    if shifted_type == 0 or shifted_type == 1 then

      -- this has a sub-sequence number
      local second_word = buf(4, 4):le_uint()
      subtree:add(f_control_sub_sequence, bit32.band(second_word, SEQUENCE_NUMBER_MASK))

      local data_index = 8

      if shifted_type == 0 then
        -- if this is an ACK let's read out the sequence number
        local sequence_number = buf(8, 4):le_uint()
        subtree:add(f_ack_sequence_number, bit32.band(sequence_number, SEQUENCE_NUMBER_MASK))

        data_index = data_index + 4
      end

      data_length = buf:len() - data_index

      -- set the data from whatever is left in the packet
      subtree:add(f_data, buf(data_index, data_length))

    elseif shifted_type == 2 then
      -- this is a Light ACK let's read out the sequence number
      local sequence_number = buf(4, 4):le_uint()
      subtree:add(f_ack_sequence_number, bit32.band(sequence_number, SEQUENCE_NUMBER_MASK))

      data_length = buf:len() - 4

      -- set the data from whatever is left in the packet
      subtree:add(f_data, buf(4, data_length))
    elseif shifted_type == 3 or shifted_type == 4 then
     if buf:len() <= 12 then
       -- this is a NAK  pull the sequence number or range
       local sequence_number = buf(4, 4):le_uint()
       subtree:add(f_nak_sequence_number, bit32.band(sequence_number, SEQUENCE_NUMBER_MASK))

       data_length = buf:len() - 4

       if buf:len() > 8 then
         local range_end = buf(8, 4):le_uint()
         subtree:add(f_nak_range_end, bit32.band(range_end, SEQUENCE_NUMBER_MASK))

         data_length = data_length - 4
       end
     end
    else
      data_length = buf:len() - 4

      -- no sub-sequence number, just read the data
      subtree:add(f_data, buf(4, data_length))
    end
  else
    -- dissect the data packet
    pinfo.cols.protocol = p_hfudt.name

    -- set the reliability bit
    subtree:add(f_reliable_bit, bit32.rshift(first_word, 30))

    local message_bit = bit32.band(0x01, bit32.rshift(first_word, 29))

    -- set the message bit
    subtree:add(f_message_bit, message_bit)

    -- read the obfuscation level
    local obfuscation_bits = bit32.band(0x03, bit32.rshift(first_word, 27))
    subtree:add(f_obfuscation_level, obfuscation_bits)

    -- read the sequence number
    subtree:add(f_sequence_number, bit32.band(first_word, SEQUENCE_NUMBER_MASK))

    local payload_offset = 4

    -- if the message bit is set, handle the second word
    if message_bit == 1 then
      payload_offset = 12

      local second_word = buf(4, 4):le_uint()

      -- read message position from upper 2 bits
      local message_position = bit32.rshift(second_word, 30)
      local position = subtree:add(f_message_position, message_position)

      if message_positions[message_position] ~= nil then
        -- if we know this position then add the name
        position:append_text(" (".. message_positions[message_position] .. ")")
      end

      -- read message number from lower 30 bits
      subtree:add(f_message_number, bit32.band(second_word, 0x3FFFFFFF))

      -- read the message part number
      subtree:add(f_message_part_number, buf(8, 4):le_uint())
    end

    -- read the type
    local packet_type = buf(payload_offset, 1):le_uint()
    local ptype = subtree:add_le(f_type, buf(payload_offset, 1))
    local packet_type_text = packet_types[packet_type]
    if packet_type_text ~= nil then
      subtree:add(f_type_text, packet_type_text)
      -- if we know this packet type then add the name
      ptype:append_text(" (".. packet_type_text .. ")")
    end

    -- read the version
    subtree:add_le(f_version, buf(payload_offset + 1, 1))

    local i = payload_offset + 2

    if unsourced_packet_types[packet_type_text] == nil then
      -- read node local ID
      local sender_id = buf(payload_offset + 2, 2)
      subtree:add_le(f_sender_id, sender_id)
      i = i + 2

      -- read HMAC MD5 hash
      subtree:add(f_hmac_hash, buf(i, 16))
      i = i + 16
    end

    -- Domain packets
    if packet_type_text == "DomainList" then
      Dissector.get("hf-domain"):call(buf(i):tvb(), pinfo, tree)
    end

    -- AvatarData or BulkAvatarDataPacket
    if packet_type_text == "AvatarData" or packet_type_text == "BulkAvatarData" then
      Dissector.get("hf-avatar"):call(buf(i):tvb(), pinfo, tree)
    end

    if packet_type_text == "EntityEdit" then
      Dissector.get("hf-entity"):call(buf(i):tvb(), pinfo, tree)
    end

    if packet_types[packet_type] == "MicrophoneAudioNoEcho" or
       packet_types[packet_type] == "MicrophoneAudioWithEcho" or
       packet_types[packet_type] == "SilentAudioFrame" then
      Dissector.get("hf-audio"):call(buf(i):tvb(), pinfo, tree)
    end
  end

  -- return the size of the header
  return buf:len()

end

function p_hfudt.init()
  local udp_dissector_table = DissectorTable.get("udp.port")

  for port=1000, 65000 do
    udp_dissector_table:add(port, p_hfudt)
  end
end
