// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
namespace LedMatrixDebugConsole
{
    public class AccelerometerStatePacket : ReceivedComPacket
    {
        public AccelerometerStatePacket(bool isAccelerometerOn)
        {
            IsAccelerometerOn = isAccelerometerOn;
        }

        public bool IsAccelerometerOn { get; set; }
    }

    public class AccelerometerStatePacketType : IReceivedComPacketType
    {
        public event OnReceivedComPacketHandler Packet;

        public int PacketLength { get { return 1; } }

        public bool CheckPacketSignature(byte signature)
        {
            return (signature & 0xF0) == 0xE0;
        }

        public void ParsePacket(byte[] data)
        {
            var handler = Packet;
            if (handler != null)
            {
                handler(this, new ReceivedComPacketEventArgs(
                    new AccelerometerStatePacket((data[0] & 0x01) == 0x01)));
            }
        }
    }
}
