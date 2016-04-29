// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;

namespace LedMatrixDebugConsole
{
    public class SetNumericDisplayValuePacket : IComPacketToSend
    {
        public UInt32 Value { get; set; }

        public SetNumericDisplayValuePacket()
        {
            Value = 0;
        }

        public byte[] GetRawData()
        {
            var data = new byte[5] { 0xff, 0x06, 0, 0, 0 };
            data[2] = (byte)(Value & 0xff);
            data[3] = (byte)((Value >> 8) & 0xff);
            data[4] = (byte)((Value >> 16) & 0xff);
            return data;
        }
    }
}
