using System;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using Avalonia.Controls;
using Avalonia.Interactivity;
using MsBox.Avalonia;

namespace Editor.Views;

public partial class MainWindow : Window
{
    private Socket? _socket; // ! - null-forgiving operator
    private const string ServerIp = "localhost";
    private const int ServerPort = 5234;

    public MainWindow()
    {
        InitializeComponent();
        InitializeSocket();
    }

    private async void InitializeSocket()
    {
        try
        {
            _socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream,
                ProtocolType.Tcp);
            await _socket.ConnectAsync(ServerIp, ServerPort);

            // What should realistically happening but is not in order to avoid warnings
            // Task.Run adds a new task to the thread pool
            // Task.Run(() => socketListener.ListenForMessages(_socket));
            async void StartListening() => await SocketListener.ListenForMessages(_socket);
            new Thread(StartListening).Start();

            await MessageBoxManager.GetMessageBoxStandard(
                "Success", "Connected to server").ShowWindowAsync();
        }
        catch (Exception ex)
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Couldn't connect to server\n" + ex.Message).ShowWindowAsync();
        }
    }

    private async void DataSend_OnClick(object sender, RoutedEventArgs routedEventArgs)
    {
        if (_socket == null)
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Socket isn't connected to server").ShowWindowAsync();
            return;
        }

        var dataToSend = SendDataTextBox.Text;
        if (dataToSend == null)
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Please enter some data to send").ShowWindowAsync();
            return;
        }

        var dataToSendBytes = Encoding.ASCII.GetBytes(dataToSend);
        try
        {
            await _socket.SendAsync(dataToSendBytes);
        }

        catch (Exception ex)
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Couldn't send message to server\n" + ex.Message).ShowWindowAsync();
        }
    }
}