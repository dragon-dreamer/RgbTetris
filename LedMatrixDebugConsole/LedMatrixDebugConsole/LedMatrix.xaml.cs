// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System.Collections.ObjectModel;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace LedMatrixDebugConsole
{
    /// <summary>
    /// Interaction logic for LedMatrix.xaml
    /// </summary>
    public partial class LedMatrix : UserControl
    {
        private readonly ObservableCollection<ObservableCollection<ColorValue>> _matrixData =
            new ObservableCollection<ObservableCollection<ColorValue>>();
        private static readonly DependencyProperty HorizontalCountProperty =
            DependencyProperty.Register("HorizontalCount", typeof(int),
            typeof(LedMatrix), new PropertyMetadata(new PropertyChangedCallback(MatrixSizeChanged)));
        private static readonly DependencyProperty VerticalCountProperty =
            DependencyProperty.Register("VerticalCount", typeof(int),
            typeof(LedMatrix), new PropertyMetadata(new PropertyChangedCallback(MatrixSizeChanged)));

        public LedMatrix()
        {
            SetValue(HorizontalCountProperty, 1);
            SetValue(VerticalCountProperty, 1);
            CreateMatrix();
            InitializeComponent();
            MatrixContents.ItemsSource = _matrixData;
        }

        public ObservableCollection<ObservableCollection<ColorValue>> MatrixData
        {
            get { return _matrixData; }
        }

        public Color[,] GetMatrixData()
        {
            var ret = new Color[VerticalCount, HorizontalCount];

            for (int y = 0; y != VerticalCount; ++y)
                for (int x = 0; x != HorizontalCount; ++x)
                    ret[y, x] = _matrixData[y][x].Value;

            return ret;
        }

        public int HorizontalCount
        {
            get { return (int)GetValue(HorizontalCountProperty); }
            set
            {
                SetValue(HorizontalCountProperty, value);
                CreateMatrix();
            }
        }

        public int VerticalCount
        {
            get { return (int)GetValue(VerticalCountProperty); }
            set
            {
                SetValue(VerticalCountProperty, value);
                CreateMatrix();
            }
        }

        private void CreateMatrix()
        {
            _matrixData.Clear();
            for (int y = 0; y != VerticalCount; ++y)
            {
                _matrixData.Add(new ObservableCollection<ColorValue>());
                for (int x = 0; x != HorizontalCount; ++x)
                {
                    _matrixData[y].Add(new ColorValue { Value = Colors.Black });
                }
            }
        }

        static private void MatrixSizeChanged(DependencyObject d, DependencyPropertyChangedEventArgs e)
        {
            LedMatrix matrix = (LedMatrix)d;
            matrix.CreateMatrix();
        }
    }
}
