-- Custom Wireshark Dissector for BCMP
local proto_bcmp = Proto("bcmp", "Bristlemouth Control Message Protocol")

local message_types = {"Heartbeat", "Echo Request", "Echo Reply", "Info Request", "Info Reply", "Capabilities Request",
                       "Capabilities Reply", "Neighbor Table Request", "Neighbor Table Reply", "Resource Table Request",
                       "Resource Table Reply", "Neighbor Proto Request", "Neighbor Proto Reply"}
message_types[0x10] = "Time Request"
message_types[0x11] = "Time Reply"
message_types[0x12] = "Time Set"
message_types[0xA0] = "Config Get"
message_types[0xA1] = "Config Value"
message_types[0xA2] = "Conifig Set"
message_types[0xA3] = "Config Commit"
message_types[0xA4] = "Config Status Request"
message_types[0xA5] = "Config Status Reply"
message_types[0xA6] = "Config Delete Request"
message_types[0xA7] = "Config Delete Reply"
message_types[0xB0] = "Net Stat Request"
message_types[0xB1] = "Net Stat Reply"
message_types[0xB2] = "Power Stat Request"
message_types[0xB3] = "Power Stat Reply"
message_types[0xC0] = "Reboot Request"
message_types[0xC1] = "Reboot Reply"
message_types[0xC2] = "Net Assert Quiet"
message_types[0xD0] = "DFU Start"
message_types[0xD1] = "DFU Payload Request"
message_types[0xD2] = "DFU Payload"
message_types[0xD3] = "DFU End"
message_types[0xD4] = "DFU Ack"
message_types[0xD5] = "DFU Abort"
message_types[0xD6] = "DFU Heartbeat"
message_types[0xD7] = "DFU Reboot Request"
message_types[0xD8] = "DFU Reboot"
message_types[0xD9] = "DFU Boot Complete"

-- Define protocol fields
local fields = proto_bcmp.fields
fields.type = ProtoField.uint16("bcmp.type", "Type", base.HEX, message_types)
fields.checksum = ProtoField.uint16("bcmp.checksum", "Checksum", base.HEX)
fields.flags = ProtoField.uint8("bcmp.flags", "Flags", base.HEX)
fields.reserved = ProtoField.uint8("bcmp.reserved", "Reserved", base.HEX)
fields.seq_num = ProtoField.uint32("bcmp.seq_num", "Sequence #", base.DEC)
fields.frag_total = ProtoField.uint8("bcmp.frag_total", "Total Fragments", base.DEC)
fields.frag_id = ProtoField.uint8("bcmp.frag_id", "Fragment ID", base.DEC)
fields.next_header = ProtoField.uint8("bcmp.next_header", "Next Header", base.HEX)
fields.data = ProtoField.bytes("bcmp.data", "Data")

-- Dissector function
function proto_bcmp.dissector(tvb, pinfo, tree)
    -- Check packet length
    if tvb:captured_len() < 13 then
        return 0
    end

    -- Create protocol tree
    local subtree = tree:add(proto_bcmp, tvb())

    -- Add protocol fields to the tree
    local msg_type = message_types[tvb(0, 2):le_uint()]
    subtree:add_packet_field(fields.type, tvb(0, 2), ENC_LITTLE_ENDIAN)

    local checksum = tvb(2, 2):le_uint()
    subtree:add_packet_field(fields.checksum, tvb(2, 2), ENC_LITTLE_ENDIAN)

    subtree:add_packet_field(fields.flags, tvb(4, 1), ENC_LITTLE_ENDIAN)
    subtree:add_packet_field(fields.reserved, tvb(5, 1), ENC_LITTLE_ENDIAN)
    subtree:add_packet_field(fields.seq_num, tvb(6, 4), ENC_LITTLE_ENDIAN)
    subtree:add_packet_field(fields.frag_total, tvb(10, 1), ENC_LITTLE_ENDIAN)
    subtree:add_packet_field(fields.frag_id, tvb(11, 1), ENC_LITTLE_ENDIAN)
    subtree:add_packet_field(fields.next_header, tvb(12, 1), ENC_LITTLE_ENDIAN)
    subtree:add_packet_field(fields.data, tvb(13), ENC_LITTLE_ENDIAN)

    -- Set column information
    pinfo.cols.protocol = "BCMP"
    pinfo.cols.info = string.format("%s Checksum=0x%02x", msg_type, checksum)

    return tvb:captured_len()
end

-- Register the dissector for IPv6 protocol number 188 (0xBC)
local ipv6_proto_table = DissectorTable.get("ip.proto")
ipv6_proto_table:add(0xBC, proto_bcmp)
