using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using Avalonia.Media;
using AvaloniaEdit.Highlighting;

namespace Editor.Utilities;

public class DictionaryWordsHighlighter : IHighlightingDefinition
{
    public string Name => "DictionaryWordsHighlighter";
    public HighlightingRuleSet MainRuleSet { get; }

    public HighlightingRuleSet GetNamedRuleSet(string name) => null;

    public HighlightingColor GetNamedColor(string name) => null;

    public HighlightingRuleSet GetRuleSet(HighlightingSpan span) => null;

    public IEnumerable<HighlightingColor> NamedHighlightingColors { get; } = new List<HighlightingColor>();
    public IDictionary<string, string> Properties { get; } = new Dictionary<string, string>();

    private readonly HashSet<string> _dictionary;

    public DictionaryWordsHighlighter(HashSet<string> wordsDictionary)
    {
        MainRuleSet = new HighlightingRuleSet();
        _dictionary = wordsDictionary;
        MakeRegexFromDictionary();
    }
    
    public void AddArrayToDictionary(IEnumerable<string> words)
    {
        foreach (var word in words)
        {
            _dictionary.Add(word);
        }
        MakeRegexFromDictionary();
    }
    
    public void RemoveWordFromDictionary(string word)
    {
        _dictionary.Remove(word);
        MakeRegexFromDictionary();
    }

    public void AddWordToDictionary(string word)
    {
        _dictionary.Add(word);
        MakeRegexFromDictionary();
    }

    private void MakeRegexFromDictionary()
    {
        MainRuleSet.Rules.Clear();

        MainRuleSet.Rules.Add(new HighlightingRule
        {
            Regex = new Regex("\\b(?!" + string.Join("|", _dictionary.Select(Regex.Escape)) + "\\b)\\w+",
                RegexOptions.IgnoreCase),
            Color = new HighlightingColor
            {
                Foreground = new SimpleHighlightingBrush(Colors.Green)
            }
        });
    }
}