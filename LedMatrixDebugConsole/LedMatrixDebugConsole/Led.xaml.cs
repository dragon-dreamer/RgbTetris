// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using Xceed.Wpf.Toolkit;

namespace LedMatrixDebugConsole
{
    /// <summary>
    /// Interaction logic for Led.xaml
    /// </summary>
    public partial class Led : UserControl
    {
        private ObservableCollection<ColorItem> _colorList;
        private readonly RelayCommand _openColorPickerCommand;
        private static readonly DependencyProperty LightColorProperty =
            DependencyProperty.Register("LightColor", typeof(ColorValue), typeof(Led), new PropertyMetadata(null));
        private static readonly DependencyProperty IsPopupOpenProperty =
            DependencyProperty.Register("IsPopupOpen", typeof(bool), typeof(Led), new PropertyMetadata(null));
        private static readonly DependencyProperty IsEditableProperty =
            DependencyProperty.Register("IsEditable", typeof(bool), typeof(Led), new PropertyMetadata(null));

        public Led()
        {
            _openColorPickerCommand = new RelayCommand(param => this.OpenColorPicker(),
                param => IsEditable);
            IsEditable = true;
            PopulateColorList();
            InitializeComponent();
        }

        public ObservableCollection<ColorItem> ColorList
        {
            get { return _colorList; }
        }

        public ColorValue LightColor
        {
            get { return (ColorValue)GetValue(LightColorProperty); }
            set { SetValue(LightColorProperty, value); }
        }

        public bool IsPopupOpen
        {
            get { return (bool)GetValue(IsPopupOpenProperty); }
            set { SetValue(IsPopupOpenProperty, value); }
        }

        public bool IsEditable
        {
            get { return (bool)GetValue(IsEditableProperty); }
            set { SetValue(IsEditableProperty, value); }
        }

        public ICommand OpenColorPickerCommand
        {
            get { return _openColorPickerCommand; }
        }

        private void PopulateColorList()
        {
            _colorList = new ObservableCollection<ColorItem> {
                new ColorItem(Colors.White, "White"),
                new ColorItem(Colors.Gray, "Gray"),
                new ColorItem(Colors.Black, "Black"),
                new ColorItem(Colors.Red, "Red"),
                new ColorItem(Colors.Blue, "Blue"),
                new ColorItem(Colors.Yellow, "Yellow"),
                new ColorItem(Colors.Green, "Green"),
                new ColorItem(Colors.Brown, "Brown"),
                new ColorItem(Colors.Pink, "Pink"),
                new ColorItem(Colors.Violet, "Violet") };
        }

        private void OpenColorPicker()
        {
            IsPopupOpen = true;
        }
    }
}
