// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;

namespace LedMatrixDebugConsole
{
    public class AccelerometerValuesPacket : ReceivedComPacket
    {
        public AccelerometerValuesPacket(Int16 x, Int16 y, Int16 z)
        {
            X = x;
            Y = y;
            Z = z;
        }

        public Int16 X { get; set; }
        public Int16 Y { get; set; }
        public Int16 Z { get; set; }
    }

    public class AccelerometerValuesPacketType : IReceivedComPacketType
    {
        public event OnReceivedComPacketHandler Packet;

        public int PacketLength { get { return 7; } }

        public bool CheckPacketSignature(byte signature)
        {
            return signature == 0x05;
        }

        public void ParsePacket(byte[] data)
        {
            var handler = Packet;
            if (handler == null)
                return;

            Int16 x = (Int16)((data[2] << 8) | data[1]);
            Int16 y = (Int16)((data[4] << 8) | data[3]);
            Int16 z = (Int16)((data[6] << 8) | data[5]);

            handler(this, new ReceivedComPacketEventArgs(new AccelerometerValuesPacket(x, y, z)));
        }
    }
}
