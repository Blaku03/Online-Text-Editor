using System;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using Editor.Utilities;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;

namespace Editor.Views;

public partial class Editor : Window
{
    private readonly Socket _socket;
    private Paragraph[]? _paragraphs;
    private int _currentLineNumber;
    private int _currentColumnNumber;

    public Editor(Socket socket)
    {
        InitializeComponent();
        _socket = socket;
        GetFileFromServer();
        MainEditor.TextArea.Caret.PositionChanged += Caret_PositionChanged;
        MainEditor.AddHandler(InputElement.KeyDownEvent, Text_KeyDown, RoutingStrategies.Tunnel);
        MainEditor.AddHandler(InputElement.KeyUpEvent, Text_KeyUp, RoutingStrategies.Tunnel);
        // MainEditor.TextArea.KeyDown += Text_DocumentChanged;
    }

    private async void GetFileFromServer()
    {
        // Firstly get metadata from server
        var metadata = new byte[1024];
        await _socket.ReceiveAsync(metadata);
        var metadataString = Encoding.ASCII.GetString(metadata);
        // Parsing the metadata
        // Server metadata format: file_size,ChunkSize
        var metadataArray = metadataString.Split(',');
        int fileSize, chunkSize;
        try
        {
            fileSize = int.Parse(metadataArray[0]);
            chunkSize = int.Parse(metadataArray[1]);
        }
        catch (Exception e)
        {
            await _socket.SendAsync(Encoding.ASCII.GetBytes("Error getting metadata"));
            return;
        }

        await _socket.SendAsync(Encoding.ASCII.GetBytes("OK"));

        var buffer = new byte[chunkSize];
        StringBuilder fileContent = new();

        // Receive the file
        while (fileSize > 0)
        {
            var bytesRead = await _socket.ReceiveAsync(new ArraySegment<byte>(buffer));
            fileSize -= bytesRead;
            fileContent.Append(Encoding.ASCII.GetString(buffer).Replace("\r\n", "\n"));
        }

        _paragraphs = Paragraph.GetParagraphs(fileContent.ToString());
        _paragraphs[0].IsLocked = true;
        OpenFileInEditor();
    }

    private async Task DisconnectFromServer()
    {
        try
        {
            if (_socket.Connected)
            {
                _paragraphs = null;
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

    private void OpenFileInEditor()
    {
        MainEditor.Text = Paragraph.GetContent(_paragraphs);
    }

    private async void Exit_OnClick(object sender, RoutedEventArgs e)
    {
        var box = MessageBoxManager.GetMessageBoxStandard("", "Are you sure you want to close the editor?",
            ButtonEnum.YesNo);
        var result = await box.ShowAsync();
        if (result != ButtonResult.Yes) return;
        await DisconnectFromServer();
        Close();
    }


    private async void Disconnect_OnClick(object sender, RoutedEventArgs e)
    {
        await DisconnectFromServer();
        new MainMenu().Show();
        Close();
    }

    private void Caret_PositionChanged(object? sender, EventArgs? e)
    {
        _currentLineNumber = MainEditor.TextArea.Caret.Line;
    }

    // TODO: Known bugs:
    // 1. When a line is about to be deleted but the line above is locked then then line is not deleted
    // 2. Adding lines above locked paragraph just messes up the lock status

    private void Text_KeyDown(object? sender, KeyEventArgs e)
    {
        Console.WriteLine("hi backslash");
    }

    private void Text_KeyUp(object? sender, KeyEventArgs  e)
    {
        Console.WriteLine("hi");
        if (_paragraphs == null) return;

        // New line is created or paragraph is not locked
        if (_currentLineNumber - 1 >= _paragraphs.Length || !_paragraphs[_currentLineNumber - 1].IsLocked)
        {
            _paragraphs = Paragraph.GetParagraphs(MainEditor.Text.Replace("\r\n","\n"), _paragraphs);
            return;
        }

        _currentColumnNumber = MainEditor.TextArea.Caret.Column;
        MainEditor.Text = Paragraph.GetContent(_paragraphs);
        MainEditor.TextArea.Caret.Column = _currentColumnNumber;
    }
}