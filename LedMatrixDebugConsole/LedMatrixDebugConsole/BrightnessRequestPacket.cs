// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
namespace LedMatrixDebugConsole
{
    public class BrightnessRequestPacket : IComPacketToSend
    {
        public byte[] GetRawData()
        {
            return new byte[] { 0xff, 0x01 };
        }
    }
}
