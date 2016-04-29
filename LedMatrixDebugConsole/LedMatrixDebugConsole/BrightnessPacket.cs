// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
namespace LedMatrixDebugConsole
{
    public class BrightnessPacket : ReceivedComPacket
    {
        public BrightnessPacket(byte brightness)
        {
            Brightness = brightness;
        }

        public int Brightness { get; set; }
    }

    public class BrightnessPacketType : IReceivedComPacketType
    {
        public event OnReceivedComPacketHandler Packet;

        public int PacketLength { get { return 2; } }

        public bool CheckPacketSignature(byte signature)
        {
            return signature == 0x01;
        }

        public void ParsePacket(byte[] data)
        {
            var handler = Packet;
            if (handler != null)
                handler(this, new ReceivedComPacketEventArgs(new BrightnessPacket(data[1])));
        }
    }
}
