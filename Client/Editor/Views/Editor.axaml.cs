using System;
using System.Net.Sockets;
using Avalonia.Controls;
using Avalonia.Interactivity;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;

namespace Editor.Views;

public partial class Editor : Window
{
    private readonly Socket _socket;

    private string
        _fileNameServer = null!; // Temporarily will be suppressing null assuming it will be initialized eventually

    public Editor(Socket socket)
    {
        InitializeComponent();
        _socket = socket;
        GetFileFromServer();
        OpenFileInEditor();
    }

    private async void GetFileFromServer()
    {
        // Firstly get metadata from server
        var metadata = new byte[1024];
        await _socket.ReceiveAsync(metadata);
        var metadataString = System.Text.Encoding.ASCII.GetString(metadata);
        // Parsing the metadata
        // Server metadata format: file_size,file_extension,ChunkSize
        var metadataArray = metadataString.Split(',');
        _fileNameServer = metadataArray[1];
        int fileSize, chunkSize;
        try
        {
            fileSize = int.Parse(metadataArray[0]);
            chunkSize = int.Parse(metadataArray[2]);
        }
        catch (Exception e)
        {
            await _socket.SendAsync(System.Text.Encoding.ASCII.GetBytes("Error getting metadata"));
            return;
        }

        await _socket.SendAsync(System.Text.Encoding.ASCII.GetBytes("OK"));

        // Create a new file
        // await using => will automatically dispose the file after the block even if exception is thrown
        await using var file = new System.IO.FileStream(_fileNameServer, System.IO.FileMode.Create);
        var buffer = new byte[chunkSize];

        // Receive the file
        while (fileSize > 0)
        {
            var bytesRead = await _socket.ReceiveAsync(new ArraySegment<byte>(buffer));
            fileSize -= bytesRead;
            // Thanks to buffer.AsMemory(0, bytesRead) we work more efficiently with slices of the buffer
            await file.WriteAsync(buffer.AsMemory(0, bytesRead));
        }
    }
    
    private async void DisconnectFromServer()
    {
        try
        {
            if (_socket.Connected)
            {
                _socket.Shutdown(SocketShutdown.Both);
                _socket.Close();
                await MessageBoxManager.GetMessageBoxStandard(
                    "Success", "Disconnected from server").ShowWindowAsync();
            }
            else
            {
                await MessageBoxManager.GetMessageBoxStandard(
                    "Info", "No active connection to disconnect").ShowWindowAsync();
            }
        }
        catch (Exception ex)
        {
            await MessageBoxManager.GetMessageBoxStandard(
                "Error", "Disconnection from server was unsuccessful. Try again.\n" + ex.Message).ShowWindowAsync();
        }
    }
    
    private async void Exit_OnClick(object sender, RoutedEventArgs e)
    {
        var box = MessageBoxManager.GetMessageBoxStandard("", "Are you sure you want to close the editor?", ButtonEnum.YesNo);
        var result = await box.ShowAsync();
        if (result == ButtonResult.Yes)
        {
            DisconnectFromServer();
            Close();
        }
    }
    
    private void OpenFileInEditor()
    {
        // Open the file in the editor
        var fileContent = System.IO.File.ReadAllText(_fileNameServer);
        MainEditor.Text = fileContent;
    }
}