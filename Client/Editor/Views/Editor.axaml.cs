using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.IO;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.Shapes;
using Avalonia.Input;
using Avalonia.Interactivity;
using Avalonia.Media;
using AvaloniaEdit.Editing;
using Editor.Utilities;
using MsBox.Avalonia;
using MsBox.Avalonia.Enums;


namespace Editor.Views;

public partial class Editor : Window
{
    private readonly Socket _socket;
    private readonly string _userName;
    private int _longestUsername = 0;
    private LinkedList<Paragraph>? Paragraphs { get; set; }
    private readonly DictionaryWordsHighlighter? _highlighter;

    public int CaretLine
    {
        get => MainEditor.TextArea.Caret.Line;
        set => MainEditor.TextArea.Caret.Line = value;
    }

    private int CaretColumn
    {
        get => MainEditor.TextArea.Caret.Column;
        set => MainEditor.TextArea.Caret.Column = value;
    }

    private bool _keyHandled;
    private bool _isDisconnected;

    public enum ProtocolId
    {
        SyncParagraph = 1,
        AsyncNewParagraph,
        AsyncDeleteParagraph,
        UnlockParagraph,
    }

    //flags for stopping thread's tasks
    private readonly CancellationTokenSource _cancellationTokenForSyncParagraphSending = new();
    private readonly CancellationTokenSource _cancellationTokenForServerListener = new();


    public Editor(Socket socket, string userName)
    {
        InitializeComponent();
        _socket = socket;
        _userName = userName;
        this.CanResize = false;
        _highlighter = new DictionaryWordsHighlighter(new HashSet<string>());
        MainEditor.SyntaxHighlighting = _highlighter;
        GetFileFromServer();
        MainEditor.AddHandler(InputElement.KeyDownEvent, Text_KeyDown, RoutingStrategies.Tunnel);
        MainEditor.AddHandler(InputElement.KeyUpEvent, Text_KeyUp, RoutingStrategies.Tunnel);
        MainEditor.TextArea.AddHandler(InputElement.PointerPressedEvent, TextArea_PointerPressed,
            RoutingStrategies.Tunnel);
        MainEditor.TextArea.AddHandler(InputElement.PointerReleasedEvent,
            (_, _) => { SetLockUserLine(_userName, CaretLine); },
            RoutingStrategies.Tunnel);

        // Disable selection of text
        MainEditor.AddHandler(InputElement.PointerMovedEvent,
            (sender, e) => { MainEditor.TextArea.Selection = Selection.Create(MainEditor.TextArea, 0, 0); },
            RoutingStrategies.Tunnel);
    }


    private async void GetFileFromServer()
    {
        // Firstly get metadata from server
        var metadata = new byte[10024];
        await _socket.ReceiveAsync(metadata);
        var metadataString = Encoding.ASCII.GetString(metadata);
        // Parsing the metadata
        // Server metadata format: file_size,ChunkSize
        var metadataArray = metadataString.Split(',');
        int fileSize, chunkSize, numberOfConnectedClients;
        try
        {
            fileSize = int.Parse(metadataArray[0]);
            chunkSize = int.Parse(metadataArray[1]);
            numberOfConnectedClients = int.Parse(metadataArray[2]);
        }
        catch (Exception e)
        {
            await _socket.SendAsync(Encoding.ASCII.GetBytes("Error getting metadata"));
            return;
        }

        if (numberOfConnectedClients == 0)
        {
            // TODO: handle file picker dialog
            // var filePickerWindow = new FilePickerWindow();
            // ShowDialog(filePickerWindow);
            //
            // switch (filePickerWindow.PickedOption)
            // {
            //     case 1:
            //         Console.WriteLine("Custom");
            //         break;
            //     case 2:
            //         Console.WriteLine("default");
            //         await ReceiveDefaultFileFromServer(chunkSize, fileSize);
            //         break;
            // }
        }
        else
        {
            await ReceiveDefaultFileFromServer(chunkSize, fileSize);
        }

        // Here getting the dictionary string from the server
        // in this example it will be string array
        string[] dictionaryArray = ["horse", "batman", "joe"];
        _highlighter!.AddArrayToDictionary(dictionaryArray);

        await _socket.SendAsync(Encoding.ASCII.GetBytes(_userName));

        AddLockUser(_userName, Brushes.Red);
        _longestUsername = Math.Max(_longestUsername, _userName.Length);

        // starting thread for sync sending paragraphs to the server
        Thread synchronousParagraphSending = new(() =>
        {
            try
            {
                SyncParagraphSending.SendDataToTheServer(_socket, this, _cancellationTokenForSyncParagraphSending.Token)
                    .Wait();
            }
            catch (Exception ex)
            {
                // ignored because it raises exception when the thread is stopped by the token
            }
        });

        synchronousParagraphSending.Start();

        Thread serverListener = new(() =>
        {
            try
            {
                ServerListener.ListenServer(_socket, this, _cancellationTokenForServerListener.Token).Wait();
            }
            catch (Exception ex)
            {
                // ignored because it raises exception when the thread is stopped by the token
            }
        });

        serverListener.Start();
    }

    private async Task ReceiveDefaultFileFromServer(int chunkSize, int fileSize)
    {
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

         Paragraphs = Paragraph.GenerateFromText(fileContent.ToString());

         OpenFileInEditor();
         // Initial lock protocol
         // server sends for instance 1,5,7 it means that I should lock paragraphs with these numbers
         UpdateLockedUsers(true);
         Show();   
    }

    public Paragraph? GetParagraph(int line)
    {
        return Paragraphs?.ElementAt(line - 1);
    }


    private async Task DisconnectFromServer()
    {
        try
        {
            if (_socket.Connected)
            {
                _cancellationTokenForServerListener.CancelAsync();
                _cancellationTokenForSyncParagraphSending.CancelAsync();
                Paragraphs = null;
                _socket.Shutdown(SocketShutdown.Both);
                _socket.Close();
                _isDisconnected = true;
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
        Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
            MainEditor.Text = Paragraph.GetContent(Paragraphs));
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

    private void TextArea_PointerPressed(object? sender, PointerPressedEventArgs e)
    {
        //protocol for unlocking paragraph when line is switched by mouse
        var data = $"{(int)ProtocolId.UnlockParagraph},{CaretLine}";
        var buffer = Encoding.ASCII.GetBytes(data);
        _socket.Send(buffer);
    }

    private async void Disconnect_OnClick(object sender, RoutedEventArgs e)
    {
        await DisconnectFromServer();
        new MainMenu().Show();
        Close();
    }

    private void Text_KeyDown(object? sender, KeyEventArgs e)
    {
        var currentCaretLine = CaretLine; // remember current line to increment it when enter is pressed
        if (Paragraphs == null) return;
        if (IsSafeKey(e.Key)) return;

        _keyHandled = false;
        var caretParagraph = Paragraphs.ElementAt(CaretLine - 1);

        if (e.Key is Key.Up or Key.Down)
        {
            if (caretParagraph.IsLocked)
            {
                CaretLine += (e.Key == Key.Up) ? -1 : 1;
                e.Handled = true;
                return;
            }

            var data = $"{(int)ProtocolId.UnlockParagraph},{CaretLine},{caretParagraph.Content}\n";
            var buffer = Encoding.ASCII.GetBytes(data);
            _socket.Send(buffer);
            return;
        }


        if (e.Key == Key.Enter)
        {
            if (!Paragraph.CaretAtTheEndOfLine(caretParagraph, CaretColumn))
            {
                e.Handled = true;
                _keyHandled = true;
                return;
            }

            AddNewParagraphAfter(CaretLine);

            var data = $"{(int)ProtocolId.AsyncNewParagraph},{CaretLine},{caretParagraph.Content}\n";
            var buffer = Encoding.ASCII.GetBytes(data);
            _socket.Send(buffer);

            Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() => { CaretLine = currentCaretLine + 1; });
            e.Handled = true;
            _keyHandled = true;
            return;
        }

        // if (e.Key is Key.Insert or Key.Q)
        // {
        //     // Toggle of lock
        //     caretParagraph.IsLocked = !caretParagraph.IsLocked;
        //     e.Handled = true;
        //     Refresh();
        //     return;
        // }

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
            if (CaretLine == 1 && CaretColumn == 1)
            {
                _keyHandled = true;
                return;
            }

            if (CaretColumn == 1)
            {
                var paragraphAbove = Paragraphs.ElementAt(CaretLine - 2);
                if (paragraphAbove.IsLocked)
                {
                    e.Handled = true;
                    _keyHandled = true;
                    _keyHandled = true;
                    return;
                }

                ;

                DeleteParagraph(CaretLine);

                var data = $"{(int)ProtocolId.AsyncDeleteParagraph},{CaretLine}";
                var buffer = Encoding.ASCII.GetBytes(data);
                _socket.Send(buffer);
            }

            return;
        }

        // Disable using del if caret is at the end of the line
        if (e.Key == Key.Delete)
        {
            if (caretParagraph.Content.Length + 1 == CaretColumn)
            {
                e.Handled = true;
                _keyHandled = true;
            }
            //TODO: handle del at the end of line, now is turned off
            // need to ensure line underneath is not locked
        }
    }

    public void AddNewParagraphAfter(int paragraphNumber)
    {
        var newParagraph = new Paragraph { Content = new StringBuilder() };
        var currentNode = Paragraphs!.First;
        for (int i = 0; i < paragraphNumber - 1 && currentNode != null; i++)
        {
            currentNode = currentNode.Next;
        }

        Paragraphs.AddAfter(currentNode!, newParagraph);
        Refresh();

        Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
        {
            if (paragraphNumber < CaretLine) CaretLine++; //in case of situations other clients add new line over us
        });
    }

    public void LockParagraph(int paragraphNumber)
    {
        Paragraphs!.ElementAt(paragraphNumber - 1).IsLocked = true;
    }

    public void DeleteParagraph(int paragraphNumber)
    {
        var paragraphToDelete = Paragraphs!.ElementAt(paragraphNumber - 1);
        var paragraphAbove = Paragraphs!.ElementAt(paragraphNumber - 2);

        // concatenate content of paragraph above and paragraph to delete
        StringBuilder newContent = new();
        newContent.Append(paragraphAbove.Content);
        newContent.Append(paragraphToDelete.Content);
        Paragraphs!.Remove(paragraphToDelete);
        UpdateParagraph(paragraphNumber - 1, newContent); //update paragraph above
    }

    //handling closing window by 'x'
    protected override async void OnClosing(WindowClosingEventArgs e)
    {
        if (!_isDisconnected) await DisconnectFromServer();
        base.OnClosing(e);
    }

    private void Text_KeyUp(object? sender, KeyEventArgs e)
    {
        if (Paragraphs == null) return;
        if (IsSafeKey(e.Key)) return;
        if (e.Key is Key.Up or Key.Down or Key.Enter or Key.Back)
        {
            SetLockUserLine(_userName, CaretLine);
            return;
        }

        if (_keyHandled) return;

        //updating paragraph content after key is released
        var currentParagraph = Paragraphs.ElementAt(CaretLine - 1);
        var lines = MainEditor.Text.Split('\n'); //split MainEditor.Text into lines
        currentParagraph.Content = new StringBuilder(lines[CaretLine - 1]); // update paragraph
    }

    public void Refresh()
    {
        //Refresh method needs to be called from main UIThread
        Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
        {
            var caretCopyLine = CaretLine;
            var caretCopyColumn = CaretColumn;
            MainEditor.Text = Paragraph.GetContent(Paragraphs);
            CaretLine = caretCopyLine;
            CaretColumn = caretCopyColumn;
        });
    }

    public void UpdateParagraph(int paragraphNumber, StringBuilder newContent, bool refreshOnly = false)
    {
        if (!refreshOnly) Paragraphs!.ElementAt(paragraphNumber - 1).Content = newContent;
        Refresh();
    }

    private static bool IsSafeKey(Key key)
    {
        // Keys that won't change the content of the paragraph
        return key is Key.Left or Key.Right or Key.Home or Key.End or Key.PageUp or Key.PageDown;
    }

    public void UnlockParagraph(int paragraphNumber)
    {
        Paragraphs!.ElementAt(paragraphNumber - 1).IsLocked = false;
    }

    private void SetLockUserLine(string userName, int rowNumber)
    {
        _longestUsername = Math.Max(_longestUsername, userName.Length);
        // Find the Grid for the user
        Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
        {
            Grid? userGrid = null;
            foreach (var control in LockIndicatorsPanel.Children)
            {
                var grid = (Grid?)control;
                var textBlock = grid?.Children.OfType<TextBlock>().FirstOrDefault();
                if (textBlock != null && textBlock.Text == userName)
                {
                    userGrid = grid;
                    break;
                }
            }

            userGrid ??= AddLockUser(userName, Brushes.Red);

            // Calculate the new position
            var lineHeight = MainEditor.TextArea.TextView.DefaultLineHeight;
            var characterWidth = MainEditor.TextArea.TextView.WideSpaceWidth;
            var windowWidth = this.Width;
            const double baseSidebarWidth = 21.207757339;
            var sidebarWidth = baseSidebarWidth * _longestUsername;
            var charactersPerLine = (windowWidth - sidebarWidth) / characterWidth;
            var wrappedLines = 0;

            for (var i = 0; i < rowNumber - 1; i++)
            {
                var currentParagraphLength = Paragraphs!.ElementAt(i).Content.Length;
                // if (i == rowNumber - 1)
                // {
                //     currentParagraphLength = CaretColumn;
                // }

                if (currentParagraphLength > charactersPerLine)
                {
                    wrappedLines += (int)Math.Round(currentParagraphLength / charactersPerLine);
                }

                wrappedLines++;
            }

            var yPosition = lineHeight * wrappedLines;

            // Update the position of the Grid
            userGrid.Margin = new Thickness(0, yPosition, 0, 0);
        }).Wait();
    }

    private void DeleteLockedUsersWithoutMe()
    {
        Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
        {
            foreach (var control in LockIndicatorsPanel.Children)
            {
                var grid = (Grid)control;
                var textBlock = grid.Children.OfType<TextBlock>().FirstOrDefault();
                if (textBlock != null && textBlock.Text != _userName)
                {
                    LockIndicatorsPanel.Children.Remove(grid);
                }
            }
        });
    }


    public void UpdateLockedUsers(bool lockingLines = false, bool deletes = false)
    {
        var lockMessage = new byte[10024];
        _socket.Receive(lockMessage);
        if (lockMessage[0] == '0') return;
        var lockMessageString = Encoding.ASCII.GetString(lockMessage);
        var lockMessageArray = lockMessageString.Split(',');

        if (deletes) DeleteLockedUsersWithoutMe();

        foreach (var j in lockMessageArray)
        {
            var splitLine = j.Split(' ');
            var lineNumber = int.Parse(splitLine[0]);
            if (lineNumber == 0) break;
            SetLockUserLine(splitLine[1], lineNumber);
            if (lockingLines) LockParagraph(lineNumber);
        }
    }

    private Grid AddLockUser(string userName, ISolidColorBrush color)
    {
        var grid = new Grid();
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = GridLength.Auto });

        var textBlock = new TextBlock
        {
            Text = userName,
            FontFamily = new FontFamily("Cascadia Code,Consolas,Menlo,Monospace"),
            Foreground = Brushes.RoyalBlue,
            VerticalAlignment = Avalonia.Layout.VerticalAlignment.Top,
            HorizontalAlignment = Avalonia.Layout.HorizontalAlignment.Left
        };
        Grid.SetColumn(textBlock, 0);

        var ellipse = new Ellipse
        {
            Width = 15,
            Height = 15,
            Fill = color,
            VerticalAlignment = Avalonia.Layout.VerticalAlignment.Top,
            HorizontalAlignment = Avalonia.Layout.HorizontalAlignment.Left
        };
        Grid.SetColumn(ellipse, 1);

        grid.Children.Add(textBlock);
        grid.Children.Add(ellipse);

        LockIndicatorsPanel.Children.Add(grid);
        return grid;
    }

    private async void Save_OnClick(object sender, RoutedEventArgs e)
    {
        if (Paragraphs == null)
        {
            await MessageBoxManager.GetMessageBoxStandard("Error", "No content to save.").ShowWindowAsync();
            return;
        }

        var saveFileDialog = new SaveFileDialog
        {
            Title = "Save File",
            Filters = [new FileDialogFilter() { Name = "Text Files", Extensions = { "txt" } }]
        };

        var result = await saveFileDialog.ShowAsync(this);
        if (result != null)
        {
            try
            {
                await using (var writer = new StreamWriter(result))
                {
                    foreach (var paragraph in Paragraphs)
                    {
                        await writer.WriteAsync(paragraph.Content.ToString());
                        await writer.WriteLineAsync();
                    }
                }

                await MessageBoxManager.GetMessageBoxStandard("Success", "File saved successfully.").ShowWindowAsync();
            }
            catch (Exception ex)
            {
                await MessageBoxManager.GetMessageBoxStandard("Error", $"Failed to save file: {ex.Message}")
                    .ShowWindowAsync();
            }
        }
    }

    private async void Open_OnClick(object sender, RoutedEventArgs e)
    {
        var openFileDialog = new OpenFileDialog
        {
            Title = "Open File",
            Filters = [new FileDialogFilter() { Name = "Text Files", Extensions = { "txt" } }]
        };

        var result = await openFileDialog.ShowAsync(this);
        if (result is { Length: > 0 })
        {
            var filePath = result[0]; // first argument is the file path
            try
            {
                await using var stream = File.OpenRead(filePath);
                using var reader = new StreamReader(stream);
                var fileContent = await reader.ReadToEndAsync();
                fileContent = fileContent.Replace("\r\n", "\n");
                Paragraphs = Paragraph.GenerateFromText(fileContent);
                OpenFileInEditor();
                await MessageBoxManager.GetMessageBoxStandard("Success", "File opened successfully.")
                    .ShowWindowAsync();
            }
            catch (Exception ex)
            {
                await MessageBoxManager.GetMessageBoxStandard("Error", $"Failed to open file: {ex.Message}")
                    .ShowWindowAsync();
            }
        }
    }

    private async void New_OnClick(object sender, RoutedEventArgs e)
    {
        var box = MessageBoxManager.GetMessageBoxStandard("", "Are you sure you want to open a new file?",
            ButtonEnum.YesNo);
        var result = await box.ShowAsync();
        if (result != ButtonResult.Yes) return;
        try
        {
            Paragraphs = new LinkedList<Paragraph>();
            Paragraphs.AddLast(new Paragraph { Content = new StringBuilder() });
            OpenFileInEditor();
            await MessageBoxManager.GetMessageBoxStandard("Success", "New empty file was created.").ShowWindowAsync();
        }
        catch (Exception ex)
        {
            await MessageBoxManager.GetMessageBoxStandard("Error", $"Failed to load a new file: {ex.Message}")
                .ShowWindowAsync();
        }
    }

    private void OpenDictionarySettings_OnClick(object? sender, RoutedEventArgs e)
    {
        if (_highlighter == null) throw new Exception("Highlighter is null");
        var dictionaryWindow = new Dictionary(MainEditor.Text, _highlighter, this);
        dictionaryWindow.Show();
    }
}