/*
This file is part of Papyrus Plugin for Notepad++.

Copyright (C) 2016 Tschilkroete <tschilkroete@gmail.com> (original author)
Copyright (C) 2021 blu3mania <blu3mania@hotmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "SimpleLexerBase.hpp"

#include "..\Common\NotepadPlusPlus.hpp"

#include "..\..\external\scintilla\Accessor.h"
#include "..\..\external\scintilla\ILexer.h"
#include "..\..\external\scintilla\StyleContext.h"
#include "..\..\external\scintilla\WordList.h"

#include <list>
#include <set>
#include <string>
#include <vector>

#include <windows.h>

#define LEXER_NAME "Papyrus Script"
#define LEXER_STATUS_TEXT L"Papyrus Script"

using namespace Scintilla;

namespace papyrus {

  class Lexer : public SimpleLexerBase {
    public:
      // A class that helps with subscription to shared Lexer data, since the handling are all static, and not
      // tied to a specific Lexer instance. Fro example, restyle currently displayed document, regardless if it's
      // lexed by current Lexer instance.
      //
      class SubscriptionHelper {
        public:
          SubscriptionHelper();

        private:
          // Only when configuration file exists under Notepad++'s plugin config folder can this lexer be used
          bool isUsable() const;

          // Get current buffer ID on the given view, if it's a applicable
          npp_buffer_t getApplicableBufferIdOnView(npp_view_t view) const;

          // Restyle currently displayed document, which includes Lex and Fold
          void restyleDocument() const;
          void restyleDocument(npp_view_t view) const;

          // Hotspot click handler
          void handleHotspotClick(HWND handle, Sci_Position position) const;
      };

      Lexer();

      // Interface functions with Notepad++
      inline static char* name() { return const_cast<char*>(LEXER_NAME); }
      inline static TCHAR* statusText() { return const_cast<TCHAR*>(LEXER_STATUS_TEXT); }
      inline static ILexer* factory() { return new Lexer(); }

      // Lexer functions
      void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument* pAccess) override;
      void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position lengthDoc, int initStyle, IDocument* pAccess) override;

    protected:
      // Only when configuration file exists under Notepad++'s plugin config folder can this lexer be used
      bool isUsable() const override;

      // Get word lists pointers for instre1 & 2, type1 - 7
      const std::vector<WordList*>& getInstreWordLists() const override;
      const std::vector<WordList*>& getTypeWordLists() const override;

    private:
      // Lexer style states
      enum class State {
        Default,
        Operator,
        FlowControl,
        Type,
        Keyword,
        Keyword2,
        FoldOpen,
        FoldMiddle,
        FoldClose,
        Comment,
        CommentMultiLine,
        CommentDoc,
        Number,
        String,
        Property,
        Class,
        Function
      };

      // Defined properties in current Papyrus script
      struct Property {
        std::string name;
        Sci_Position line;
      };

      enum class TokenType {
        Identifier,
        Numeric,
        Special
      };
      struct Token {
        std::string content;
        TokenType tokenType;
        Sci_Position startPos;
      };

      // Parse a text line and tokenize each word/symbol, etc.
      std::vector<Token> tokenize(Accessor& accessor, Sci_Position line) const;

      // Colorize a word/symbol in StyleContext to a provided state based on the given token.
      void colorToken(StyleContext& styleContext, Token token, State state) const;

      // Get next character (wide char supported)
      int getNextChar(Accessor& accessor, Sci_Position& index, Sci_Position& indexNext) const;

      // If a style (from StyleContext) is a comment style defined by this lexer
      bool isComment(int style) const;

      // Private members
      //

      // Word lists for different function groups
      WordList wordListOperators;   // instre1
      WordList wordListFlowControl; // instre2
      WordList wordListTypes;       // type1
      WordList wordListKeywords;    // type2
      WordList wordListKeywords2;   // type3
      WordList wordListFoldOpen;    // type4
      WordList wordListFoldMiddle;  // type5
      WordList wordListFoldClose;   // type6

      // Provide pointers to corresponding word lists to base class
      const std::vector<WordList*> instreWordLists;
      const std::vector<WordList*> typeWordLists;

      // Cache list of lines that define properties
      std::list<Property> propertyLines;

      // Cache property names defined in current file, for better performance
      std::set<std::string> propertyNames;
  };

} // namespace