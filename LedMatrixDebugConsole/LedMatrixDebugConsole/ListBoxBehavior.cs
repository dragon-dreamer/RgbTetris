// Copyright 2016 Denis T (https://github.com/dragon-dreamer / dragondreamer [ @ ] live.com)
// SPDX-License-Identifier: GPL-3.0
using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Windows;
using System.Windows.Controls;

namespace LedMatrixDebugConsole
{
    public class ListBoxBehavior
    {
        private static readonly Dictionary<ListBox, Capture> _associations =
               new Dictionary<ListBox, Capture>();

        public static bool GetScrollOnNewItem(DependencyObject obj)
        {
            return (bool)obj.GetValue(ScrollOnNewItemProperty);
        }

        public static void SetScrollOnNewItem(DependencyObject obj, bool value)
        {
            obj.SetValue(ScrollOnNewItemProperty, value);
        }

        public static readonly DependencyProperty ScrollOnNewItemProperty =
            DependencyProperty.RegisterAttached(
                "ScrollOnNewItem",
                typeof(bool),
                typeof(ListBoxBehavior),
                new UIPropertyMetadata(false, OnScrollOnNewItemChanged));

        public static void OnScrollOnNewItemChanged(
            DependencyObject obj,
            DependencyPropertyChangedEventArgs e)
        {
            var listBox = obj as ListBox;
            if (listBox == null)
                return;

            bool oldValue = (bool)e.OldValue;
            bool newValue = (bool)e.NewValue;
            if (newValue == oldValue)
                return;

            if (newValue)
            {
                listBox.Loaded += ListBox_Loaded;
                listBox.Unloaded += ListBox_Unloaded;
                var itemsSourcePropertyDescriptor = TypeDescriptor.GetProperties(listBox)["ItemsSource"];
                itemsSourcePropertyDescriptor.AddValueChanged(listBox, ListBox_ItemsSourceChanged);
            }
            else
            {
                listBox.Loaded -= ListBox_Loaded;
                listBox.Unloaded -= ListBox_Unloaded;
                if (_associations.ContainsKey(listBox))
                    _associations[listBox].Dispose();
                var itemsSourcePropertyDescriptor = TypeDescriptor.GetProperties(listBox)["ItemsSource"];
                itemsSourcePropertyDescriptor.RemoveValueChanged(listBox, ListBox_ItemsSourceChanged);
            }
        }

        private static void ListBox_ItemsSourceChanged(object sender, EventArgs e)
        {
            var listBox = (ListBox)sender;
            if (_associations.ContainsKey(listBox))
                _associations[listBox].Dispose();
            _associations[listBox] = new Capture(listBox);
        }

        static void ListBox_Unloaded(object sender, RoutedEventArgs e)
        {
            var listBox = (ListBox)sender;
            if (_associations.ContainsKey(listBox))
            {
                _associations[listBox].Dispose();
                _associations.Remove(listBox);
            }

            listBox.Unloaded -= ListBox_Unloaded;
        }

        static void ListBox_Loaded(object sender, RoutedEventArgs e)
        {
            var listBox = (ListBox)sender;
            if (!(listBox.Items is INotifyCollectionChanged))
                return;
            listBox.Loaded -= ListBox_Loaded;
            _associations[listBox] = new Capture(listBox);
        }

        class Capture : IDisposable
        {
            private readonly ListBox _listBox;
            private readonly INotifyCollectionChanged _notifyCollectionChanged;

            public Capture(ListBox listBox)
            {
                _listBox = listBox;
                _notifyCollectionChanged = listBox.ItemsSource as INotifyCollectionChanged;
                if (_notifyCollectionChanged != null)
                    _notifyCollectionChanged.CollectionChanged += incc_CollectionChanged;
            }

            void incc_CollectionChanged(object sender, NotifyCollectionChangedEventArgs e)
            {
                if (e.Action == NotifyCollectionChangedAction.Add)
                {
                    _listBox.ScrollIntoView(e.NewItems[0]);
                    _listBox.SelectedItem = e.NewItems[0];
                }
            }

            public void Dispose()
            {
                if (_notifyCollectionChanged != null)
                    _notifyCollectionChanged.CollectionChanged -= incc_CollectionChanged;
            }
        }
    }
}
