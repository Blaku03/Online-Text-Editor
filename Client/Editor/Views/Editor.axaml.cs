using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Interactivity;
using AvaloniaEdit.Editing;
using Editor.Utilities;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;

namespace Editor.Views;

public partial class Editor : Window
{
    private readonly Socket _socket;
    private LinkedList<Paragraph>? _paragraphs;
    private int CaretLine => MainEditor.TextArea.Caret.Line;
    private int CaretColumn => MainEditor.TextArea.Caret.Column;
    private bool _keyHandled;
    
    //flags for stopping thread's tasks
    private readonly CancellationTokenSource _cancellationTokenForSyncParagraphSending = new();
    

    public Editor(Socket socket)
    {
        InitializeComponent();
        _socket = socket;
        GetFileFromServer();
        MainEditor.AddHandler(InputElement.KeyDownEvent, Text_KeyDown, RoutingStrategies.Tunnel);
        MainEditor.AddHandler(InputElement.KeyUpEvent, Text_KeyUp, RoutingStrategies.Tunnel);

        // Disable selection of text
        MainEditor.AddHandler(InputElement.PointerMovedEvent,
            (sender, e) => { MainEditor.TextArea.Selection = Selection.Create(MainEditor.TextArea, 0, 0); },
            RoutingStrategies.Tunnel);
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

        _paragraphs = Paragraph.GenerateFromText(fileContent.ToString());
        _paragraphs.ElementAt(1).IsLocked = true;
        OpenFileInEditor();
        
        // starting thread for sync sending paragraphs to the server
        Thread synchronousParagraphSending = new(() =>
        {
            try
            {
                SyncParagraphSending.SendDataToTheServer(_socket, this, _cancellationTokenForSyncParagraphSending.Token).Wait();
            }
            catch (Exception ex)
            {
                // ignored because it raises exception when the thread is stopped by the token
            }
        });

        synchronousParagraphSending.Start();
    }
    
    public Paragraph? GetParagraph(int line)
    {
        return _paragraphs?.ElementAt(line - 1);
    }

    public int GetCaretLine() => CaretLine;

    private async Task DisconnectFromServer()
    {
        try
        {
            if (_socket.Connected)
            {
                await _cancellationTokenForSyncParagraphSending.CancelAsync();
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

    // TODO: Fix deleting lines
    // Deleting line only if it's empty and not locked

    private void Text_KeyDown(object? sender, KeyEventArgs e)
    {
        if (_paragraphs == null) return;
        if (IsSafeKey(e.Key)) return;

        _keyHandled = false;
        var caretParagraph = _paragraphs.ElementAt(CaretLine - 1);

        if (e.Key == Key.Enter)
        {
            if (!Paragraph.CaretAtTheEndOfLine(caretParagraph, CaretColumn))
            {
                e.Handled = true;
                _keyHandled = true;
                return;
            }

            var newParagraph = new Paragraph { Content = new StringBuilder() };
            var currentNode = _paragraphs.First;
            for (int i = 0; i < CaretLine - 1 && currentNode != null; i++)
            {
                currentNode = currentNode.Next;
            }

            _paragraphs.AddAfter(currentNode!, newParagraph);
            MainEditor.Text = Paragraph.GetContent(_paragraphs);
            MainEditor.TextArea.Caret.Line = _paragraphs.Count + 1;
            e.Handled = true;
            _keyHandled = true;
            return;
        }

        if (e.Key is Key.Insert or Key.Q)
        {
            // Toggle of lock
            caretParagraph.IsLocked = !caretParagraph.IsLocked;
            e.Handled = true;
            var caretCopyLine = MainEditor.TextArea.Caret.Line;
            var caretCopyColumn = MainEditor.TextArea.Caret.Column;
            MainEditor.Text = Paragraph.GetContent(_paragraphs);
            MainEditor.TextArea.Caret.Line = caretCopyLine;
            MainEditor.TextArea.Caret.Column = caretCopyColumn;
            return;
        }

        // Basic lock check
        if (caretParagraph.IsLocked)
        {
            e.Handled = true;
            _keyHandled = true;
            return;
        }

        // Special handling of deleting
        if (e.Key == Key.Back)
        {
            if (caretParagraph.Content.Length == 0)
            {
                _paragraphs.Remove(caretParagraph);
                _keyHandled = true;
                return;
            }

            if (CaretColumn == 0)
            {
                e.Handled = true;
                _keyHandled = true;
            }

            return;
        }

        // Disable using del if caret is at the end of the line
        if (e.Key == Key.Delete)
        {
            if (caretParagraph.Content.Length == CaretColumn)
            {
                e.Handled = true;
                _keyHandled = true;
            }
        }
    }

    private void Text_KeyUp(object? sender, KeyEventArgs e)
    {
        if (_paragraphs == null) return;
        if (IsSafeKey(e.Key)) return;
        if (_keyHandled) return;

        // Update the content of the paragraph
        _paragraphs = Paragraph.GenerateFromText(MainEditor.Text, _paragraphs);
    }

    private bool IsSafeKey(Key key)
    {
        // Keys that won't change the content of the paragraph
        return key is Key.Up or Key.Down or Key.Left or Key.Right or Key.Home or Key.End or Key.PageUp or Key.PageDown;
    }
}