using System;
using System.Net.Sockets;
using System.Text;
using Avalonia.Controls;
using Avalonia.Interactivity;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;

namespace Editor.Views;

public partial class MainWindow : Window
{
    private Socket _socket = null!; // ! - null-forgiving operator
    private readonly string _serverIp = "localhost";
    private readonly int _serverPort = 5234;
    public MainWindow()
    {
        InitializeComponent();
        InitializeSocket();
    }
    private async void InitializeSocket()
    {
        try
        {
            _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            await _socket.ConnectAsync(_serverIp, _serverPort);

            var box = MessageBoxManager
                .GetMessageBoxStandard("Caption", "Are you sure you would like to delete appender_replace_page_1?",
                    ButtonEnum.YesNo);

            var result = await box.ShowWindowAsync();
            // MessageBox.Show("Connected to server");
        }
        catch (Exception ex)
        {
            // MessageBox.Show($"Error while connecting to server: {ex.Message}");
        }
    }

    private void DataSend_OnClick(object sender, RoutedEventArgs e)
    {
        var dataToSend = SendDataTextBox.Text;
        var dataToSendBytes = Encoding.ASCII.GetBytes(dataToSend);
        try
        {
            _socket.Send(dataToSendBytes);
        }
        catch (Exception ex)
        {
            // MessageBox.Show($"Error while sending data: {ex.Message}");
        }
    }
}