// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
namespace LedMatrixDebugConsole
{
    public class BrightnessSetupPacket : IComPacketToSend
    {
        public byte BrightnessValue { get; set; }

        public const byte MinBrightness = 20;
        public const byte MaxBrightness = 200;

        public BrightnessSetupPacket(byte brightnessValue)
        {
            BrightnessValue = brightnessValue;
            if (BrightnessValue < MinBrightness)
                BrightnessValue = MinBrightness;
            else if (BrightnessValue > MaxBrightness)
                BrightnessValue = MaxBrightness;
        }

        public byte[] GetRawData()
        {
            return new byte[] { 0xff, 0x02, BrightnessValue };
        }
    }
}
