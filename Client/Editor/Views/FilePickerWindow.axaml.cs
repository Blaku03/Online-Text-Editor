﻿using System;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Microsoft.CodeAnalysis.CSharp.Syntax;

namespace Editor.Views;

public partial class FilePickerWindow : Window
{
    public  int PickedOption { get; private set; }

    public FilePickerWindow()
    {
        InitializeComponent();
    }
    
    
    private async void OpenFilePicker_OnClick(object? sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog();
        var result = await dialog.ShowAsync(this);
        
        if (result != null)
        {
            
        }

        PickedOption = 1;
        this.Close();
    }

    private void SelectDefaultFile_OnClick(object? sender, RoutedEventArgs e)
    {
        PickedOption = 2;
        Close();
    }

}