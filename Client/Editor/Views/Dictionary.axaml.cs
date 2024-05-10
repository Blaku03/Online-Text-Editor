using System;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Editor.Utilities;

namespace Editor.Views;

public partial class Dictionary : Window
{
    private readonly DictionaryWordsHighlighter _dictionaryWordsHighlighter;
    private static readonly char[] Separator = new[] { ' ', '\n', '\r', '\t' };
    private readonly string _allWords;
    private readonly Editor _editor;

    public Dictionary(string allWords, DictionaryWordsHighlighter dictionaryWordsHighlighter, Editor editor)
    {
        InitializeComponent();
        _allWords = allWords;
        _dictionaryWordsHighlighter = dictionaryWordsHighlighter;
        _editor = editor;
    }

    private async void AddWord(object? sender, RoutedEventArgs e)
    {
        if (string.IsNullOrEmpty(WordToAdd.Text))
        {
            return;
        }

        _dictionaryWordsHighlighter.AddWordToDictionary(WordToAdd.Text);
        await _editor.SendMessage($"{(int)Editor.ProtocolId.AddKnownWord},{WordToAdd.Text}");
        WordToAdd.Text = string.Empty;
        _editor.Refresh();
    }

    private void RemoveWord(object? sender, RoutedEventArgs e)
    {
        if (string.IsNullOrEmpty(WordToRemove.Text))
        {
            return;
        }

        _dictionaryWordsHighlighter.RemoveWordFromDictionary(WordToRemove.Text);
        WordToRemove.Text = string.Empty;
        _editor.Refresh();
    }

    private void AddAllWords(object? sender, RoutedEventArgs e)
    {
        var wordsArray = _allWords.Split(Separator, StringSplitOptions.RemoveEmptyEntries);
        _dictionaryWordsHighlighter.AddArrayToDictionary(wordsArray);
        _editor.Refresh();
    }
}