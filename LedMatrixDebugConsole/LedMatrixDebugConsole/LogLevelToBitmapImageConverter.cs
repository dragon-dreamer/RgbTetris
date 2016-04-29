// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;
using System.Windows;
using System.Windows.Data;
using System.Windows.Media.Imaging;

namespace LedMatrixDebugConsole
{
    [ValueConversion(typeof(LogLevel), typeof(BitmapImage))]
    public class LogLevelToBitmapImageConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value == null || !(value is LogLevel))
                return null;

            switch ((LogLevel)value)
            {
                case LogLevel.Info:
                    return Application.Current.FindResource("InformationIcon");
                case LogLevel.Warning:
                    return Application.Current.FindResource("WarningIcon");
                case LogLevel.Error:
                    return Application.Current.FindResource("ErrorIcon");
                default:
                    return null;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotSupportedException();
        }
    }
}
