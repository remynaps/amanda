/**********************************************************************
  Author : Dick Bruin
  Date   : 19/12/99
  Version: 2.02
  File   : amlex.h

  Description:

  the lexical analyser of the Amanda interpreter

  lexicon:
  number     = (0-9) {0-9} [. {0-9}]
  identifier = (a-z | _) {a-z | A-Z | _ | 0-9} | # | ~
  typeid     = (A-Z) {a-z | A-Z | _ | 0-9}
  operator   = ispunct() {ispuntc()} | $(a-z | _) {a-z | A-Z | _ | 0-9}
  character  = '#0-#255' | '\(a|b|f|n|r|t|v|\|e|x(0..9|A..F)(0..9|A..F)'
  string     = "{#0-#255 | \(a|b|f|n|r|t|v|\|e|x(0..9|A..F)(0..9|A..F)}"
  separator  = space | newline
  comment    = / * {char} * /  |  || {#0-#255} \n
  lpar       = (
  rpar       = )
  sep        = ;
  lbrack     = [
  rbrack     = ]
  lacc       = {
  racc       = }
  bar        = |
  gener      = <-
  assignment = :=
  points     = ..
  colons     = ::
  def        = ::=
  syn        = ==
  arrow      = ->
  where      = "where"
  otherwise  = "otherwise"
  abstype    = "abstype"
  with       = "with"
  generic    = "generic"

  \e denotes the escape character
  comments may be nested

  another file may be included by a directive:
  #import "fileName" \n
*************************************************************************/

#ifndef AMLEX_H
#define AMLEX_H

#include "bool.h"

typedef enum
{ NUMBER, IDENTIFIER, TYPEID, OPERATOR, CHARACTER, STRING, LPAR, RPAR, SEP,
  COMMA, LBRACK, RBRACK, LACC, RACC, BAR, WHERE, OTHERWISE, GENER, ASSIGNMENT,
  POINTS, COLONS, DEF, SYN, ARROW, ABSTYPE, WITH, GENERIC, offside, empty
} TokenType;

extern TokenType tokentype;

extern char tokenval[];

extern int tokenindent;

extern int tokenoffside;

/**********************************************************************
  PRE:
    tokenoffside contains the offside indentation
  EFFECT:
    if the position of a new token is past tokenoffside
      a tokentype of offside is returned
    else if the current input stream is at the end
      a tokentype of empty is returned
    else
      scans a token from the current input stream
  POST:
    tokentype contains the type,
    tokenval contains the token itself,
    tokenindent contains the position of the token
***********************************************************************/
void gettoken(void);

/**********************************************************************
  initialises the lexical analyser
***********************************************************************/
void initlex(void);

/**********************************************************************
  changes the current input stream to the file <filename>
***********************************************************************/
void openfileinput(char filename[]);

/**********************************************************************
  changes the current input stream to the string s
***********************************************************************/
void openinput(char s[]);

/**********************************************************************
  closes the current input stream
  don't forget this
***********************************************************************/
void closeinput(void);

/**********************************************************************
  to be called when an error during parsing is found
  produces a message about the position of the error in the input
  in case of file input a file is made with an appropriate call of the editor
***********************************************************************/
void lexerror(void);

/**********************************************************************
  a positionCode is a code for a filename, linenr pair
***********************************************************************/
int getPositionCode(void);

char *posCodeFileName(int positionCode);
int posCodeLinenr(int positionCode);

#endif
