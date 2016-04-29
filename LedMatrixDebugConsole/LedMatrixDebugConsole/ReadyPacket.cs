// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
namespace LedMatrixDebugConsole
{
    public class ReadyPacket : ReceivedComPacket
    {
    }

    public class ReadyPacketType : IReceivedComPacketType
    {
        public event OnReceivedComPacketHandler Packet;

        public int PacketLength { get { return 1; } }

        public bool CheckPacketSignature(byte signature)
        {
            return signature == 0x78;
        }

        public void ParsePacket(byte[] data)
        {
            var handler = Packet;
            if (handler != null)
                handler(this, new ReceivedComPacketEventArgs(new ReadyPacket()));
        }
    }
}
