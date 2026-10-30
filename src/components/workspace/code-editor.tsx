"use client";

import CodeMirror from "@uiw/react-codemirror";
import { HighlightStyle, indentUnit, syntaxHighlighting } from "@codemirror/language";
import { Decoration, type DecorationSet, EditorView, ViewPlugin, type ViewUpdate, keymap } from "@codemirror/view";
import { indentLess, indentMore } from "@codemirror/commands";
import { EditorSelection, EditorState, Prec, StateEffect, StateField, type Extension } from "@codemirror/state";
import { tags as t } from "@lezer/highlight";
import { useEffect, useMemo, useRef, useState } from "react";
import type { Language } from "@/lib/curriculum/types";

// Colours come from the --code-* tokens in globals.css, so the editor follows
// the site theme (light or dark) and stays readable in both.
const C = {
  bg: "var(--code-bg)",
  gutterBg: "var(--code-bg)",
  text: "var(--code-text)",
  muted: "var(--code-muted)",
  activeLine: "var(--code-active)",
  selection: "var(--code-selection)",
  bracket: "var(--code-bracket)",
  cursor: "var(--code-cursor)",
  keyword: "var(--code-keyword)",
  type: "var(--code-type)",
  string: "var(--code-string)",
  number: "var(--code-number)",
  comment: "var(--code-comment)",
  macro: "var(--code-macro)",
  fn: "var(--code-fn)",
};

const makeTheme = (fontSize: number) => EditorView.theme(
  {
    "&": { backgroundColor: C.bg, color: C.text, fontSize: `${fontSize}px`, height: "100%" },
    ".cm-content": { fontFamily: "var(--font-mono)", caretColor: C.cursor, padding: "12px 0", lineHeight: "1.65" },
    ".cm-gutters": { backgroundColor: C.gutterBg, color: C.muted, border: "none", paddingRight: "4px" },
    ".cm-activeLine": { backgroundColor: C.activeLine },
    // Translucent, so the selection layer underneath still shows through.
    ".cm-line.cm-errorLine": { backgroundColor: "var(--err-soft)" },
    ".cm-activeLineGutter": { backgroundColor: C.activeLine, color: C.text },
    ".cm-cursor, .cm-dropCursor": { borderLeftColor: C.cursor, borderLeftWidth: "2px" },
    "&.cm-focused": { outline: "none" },
    "&.cm-focused .cm-selectionBackground, .cm-selectionBackground, .cm-content ::selection": { backgroundColor: `${C.selection} !important` },
    ".cm-matchingBracket": { backgroundColor: C.bracket, outline: `1px solid ${C.muted}` },
    ".cm-scroller": { overflow: "auto" },
    ".cm-panels": { backgroundColor: C.activeLine, color: C.text },
  },
);

const highlight = HighlightStyle.define([
  { tag: [t.keyword, t.controlKeyword, t.modifier, t.definitionKeyword, t.operatorKeyword], color: C.keyword },
  { tag: [t.typeName, t.standard(t.typeName), t.className], color: C.type },
  { tag: [t.string, t.character, t.special(t.string)], color: C.string },
  { tag: [t.number, t.bool, t.null], color: C.number },
  { tag: [t.comment, t.lineComment, t.blockComment], color: C.comment, fontStyle: "italic" },
  { tag: [t.processingInstruction, t.macroName, t.meta], color: C.macro },
  { tag: [t.function(t.variableName), t.function(t.propertyName)], color: C.fn },
]);

// Each lesson needs one grammar, so load only that one.
const loadLanguage = (language: Language): Promise<Extension> =>
  language === "c" || language === "kernel"
    ? import("@codemirror/lang-cpp").then((m) => m.cpp())
    : language === "asm"
      ? import("./asm-language").then((m) => m.asm)
      : language === "go"
        ? Promise.all([import("@codemirror/language"), import("@codemirror/legacy-modes/mode/go")]).then(([l, m]) => l.StreamLanguage.define(m.go))
        : import("@codemirror/lang-rust").then((m) => m.rust());

// CodeMirror's own active-line highlight is opaque and sits above the selection
// layer, so the cursor's line (the last line of a selection) looked unselected.
// Only highlight the line when nothing is selected.
const activeLine = ViewPlugin.fromClass(
  class {
    decorations: DecorationSet;
    constructor(view: EditorView) {
      this.decorations = this.build(view);
    }
    update(update: ViewUpdate) {
      if (update.docChanged || update.selectionSet) this.decorations = this.build(update.view);
    }
    build(view: EditorView) {
      const { state } = view;
      if (state.selection.ranges.some((r) => !r.empty)) return Decoration.none;
      return Decoration.set([Decoration.line({ class: "cm-activeLine" }).range(state.doc.lineAt(state.selection.main.head).from)]);
    }
  },
  { decorations: (plugin) => plugin.decorations },
);

const setErrorLines = StateEffect.define<number[]>();
const errorLineField = StateField.define<DecorationSet>({
  create: () => Decoration.none,
  update(lines, tr) {
    lines = lines.map(tr.changes);
    for (const effect of tr.effects) {
      if (!effect.is(setErrorLines)) continue;
      const mark = Decoration.line({ class: "cm-errorLine" });
      lines = Decoration.set(effect.value.filter((n) => n >= 1 && n <= tr.state.doc.lines).map((n) => mark.range(tr.state.doc.line(n).from)));
    }
    return lines;
  },
  provide: (field) => EditorView.decorations.from(field),
});

const NO_LANGUAGE: Extension = [];

const NO_LINES: number[] = [];

/**
 * Tab behaves like a typical code editor: with no selection it inserts spaces
 * at the cursor up to the next tab stop; with a selection it indents the
 * selected lines. Shift+Tab outdents.
 */
const makeTabKeymap = (tabWidth: number) => keymap.of([
  {
    key: "Tab",
    run: (view) => {
      const { state } = view;
      if (state.selection.ranges.some((r) => !r.empty)) return indentMore(view);
      view.dispatch(
        state.changeByRange((range) => {
          const line = state.doc.lineAt(range.head);
          const col = range.head - line.from;
          const spaces = " ".repeat(tabWidth - (col % tabWidth));
          return { changes: { from: range.head, insert: spaces }, range: EditorSelection.cursor(range.head + spaces.length) };
        }),
        { scrollIntoView: true, userEvent: "input" },
      );
      return true;
    },
    shift: indentLess,
  },
]);

export default function CodeEditor({
  value,
  onChange,
  language,
  onRun,
  onTest,
  label,
  errorLines = NO_LINES,
  fontSize = 15,
  tabWidth = 4,
}: {
  value: string;
  onChange: (v: string) => void;
  language: Language;
  onRun: () => void;
  onTest: () => void;
  label: string;
  errorLines?: number[];
  fontSize?: number;
  tabWidth?: number;
}) {
  const view = useRef<EditorView | null>(null);
  const [grammar, setGrammar] = useState<{ language: Language; ext: Extension } | null>(null);
  useEffect(() => {
    let live = true;
    loadLanguage(language).then((ext) => live && setGrammar({ language, ext }));
    return () => {
      live = false;
    };
  }, [language]);
  const languageExt = grammar?.language === language ? grammar.ext : NO_LANGUAGE;

  const theme = useMemo(() => makeTheme(fontSize), [fontSize]);
  const extensions = useMemo(
    () => [
      languageExt,
      syntaxHighlighting(highlight),
      activeLine,
      errorLineField,
      indentUnit.of(" ".repeat(tabWidth)),
      EditorState.tabSize.of(tabWidth),
      EditorView.contentAttributes.of({ "aria-label": `${label}. Tab indents; press Escape then Tab to leave the editor.` }),
      // CodeMirror's built-in escape hatch (Esc, then Tab) keeps keyboard users
      // from getting trapped in the editor.
      makeTabKeymap(tabWidth),
      Prec.highest(
        keymap.of([
          { key: "Mod-Enter", run: () => (onTest(), true) },
          { key: "Shift-Mod-Enter", run: () => (onRun(), true) },
        ]),
      ),
    ],
    [languageExt, label, onRun, onTest, tabWidth],
  );
  useEffect(() => {
    view.current?.dispatch({ effects: setErrorLines.of(errorLines) });
  }, [errorLines]);
  return (
    <CodeMirror
      value={value}
      onCreateEditor={(v) => {
        view.current = v;
        v.dispatch({ effects: setErrorLines.of(errorLines) });
      }}
      onChange={onChange}
      extensions={extensions}
      theme={theme}
      height="100%"
      className="h-full"
      basicSetup={{ foldGutter: false, highlightActiveLine: false, autocompletion: false, searchKeymap: true }}
      indentWithTab={false}
    />
  );
}
