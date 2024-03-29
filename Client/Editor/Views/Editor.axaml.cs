using System;
using System.Net.Sockets;
using Avalonia.Controls;

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

    private void OpenFileInEditor()
    {
        // Open the file in the editor
        var fileContent = System.IO.File.ReadAllText(_fileNameServer);
        MainEditor.Text = fileContent;
    }
}