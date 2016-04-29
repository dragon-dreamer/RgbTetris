// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System.Collections.Generic;
using System.Windows.Media;

namespace LedMatrixDebugConsole
{
    public class DisplayPacket : IComPacketToSend
    {
        Color[,] _data;

        public DisplayPacket(Color[,] data)
        {
            _data = data;
        }

        private void AddColor(List<byte> retData, byte color)
        {
            retData.Add(color);
            if (color == 0xff)
                retData.Add(color);
        }

        public byte[] GetRawData()
        {
            List<byte> retData = new List<byte> { 0xff, 0x00 }; //Display coord sync packet
            int ySize = _data.GetLength(0);
            int xSize = _data.GetLength(1);
            for (int y = ySize - 1; y >= 0; --y)
            {
                for(int x = xSize - 1; x >= 0; --x)
                {
                    AddColor(retData, _data[y, x].R);
                    AddColor(retData, _data[y, x].G);
                    AddColor(retData, _data[y, x].B);
                }
            }

            return retData.ToArray();
        }
    }
}
