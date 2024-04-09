using System;
using System.Linq;
using System.Net.Sockets;
using Avalonia.Controls;
using Avalonia.Interactivity;
using MsBox.Avalonia;

namespace Editor.Views;

public partial class MainMenu : Window
{
    private const string DefaultServerIp = "localhost";
    private const int DefaultServerPort = 5234;

    public MainMenu()
    {
        InitializeComponent();
        SetDefaultValues();
    }

    private void SetDefaultValues()
    {
        IpTextBox.Text = DefaultServerIp;
        PortTextBox.Text = DefaultServerPort.ToString();
    }

    private void PortTextBox_OnTextInput(object? sender, TextChangingEventArgs textChangingEventArgs)
    {
        var textBox = (TextBox)sender!;

        var currentText = textBox.Text;
        if (string.IsNullOrEmpty(currentText)) return;
        textBox.Text = string.Concat(currentText.Where(char.IsDigit));
    }

    private async void Connect_OnClick(object? sender, RoutedEventArgs e)
    {
        // Check if IP or Port are empty
        if (string.IsNullOrEmpty(IpTextBox.Text))
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Please provide ip to connect to").ShowWindowAsync();
            return;
        }

        if (string.IsNullOrEmpty(PortTextBox.Text))
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Please provide port to connect to").ShowWindowAsync();
            return;
        }

        // Check if user can connect to the server
        try
        {
            var socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream,
                ProtocolType.Tcp);
            await socket.ConnectAsync(IpTextBox.Text, int.Parse(PortTextBox.Text));
            // await MessageBoxManager.GetMessageBoxStandard(
            //     "Success", "Successfully connected to server").ShowWindowAsync();
            new Editor(socket).Show();
            Close();
        }
        catch (Exception ex)
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Couldn't connect to server\n" + ex.Message).ShowWindowAsync();
        }
    }
}