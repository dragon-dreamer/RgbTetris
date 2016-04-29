// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System.Windows.Media;

namespace LedMatrixDebugConsole
{
    public class ColorValue : PropertyChangedHelper
    {
        private Color _value;

        public Color Value
        {
            get { return _value; }
            set
            {
                if (_value != value)
                {
                    _value = value;
                    OnPropertyChanged();
                }
            }
        }
    }
}
