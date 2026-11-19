#pragma once

// commands/: THE PARSER'S CURE, R30. Sixteen rounds of if-chain sediment
// (R14's first verbs to R30's examine) become DATA. The layers: the
// tokenizer splits words and strips articles, knowing no verbs; the
// verb table owns dispatch AND generates the help line (help cannot
// drift from what the parser accepts); handlers own prose and nothing
// else. A noun registry rides this spine in R31 - things as data too.
// Behavior this round is FROZEN: same grammar, same responses, same
// saves; the proof of the surgery is that nothing observable changes
// except where the code lives.

struct Command;

struct Args {
    int         argc;           // words after the verb, articles stripped
    const char* argv[8];       // each word, C-strings in place
    const char* rest;          // the untouched remainder, for verbs of text
};

using CommandFn = void (*)(const Args& a);

struct Command {
    const char* verb;          // what the player types
    const char* aliases[4];    // shorthand, nullptr-terminated
    const char* syntax;        // help fragment: args only, no verb
    CommandFn   handler;       // uniform signature
};

void command_run(const char* line);   // tokenize, walk the table, call
void commands_help();                 // the Commands: line, GENERATED from the table
