// -----------------------------------------------------------------------------
// Analizador .brik — Lexer + Parser + AST + Serializador 1.0
// CLI:
//   analizador input.brik [-o arbol.ast] [--tokens] [--tokfile tokens.txt] [--no-ast]
// -----------------------------------------------------------------------------
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// -------------------- Aliases para reducir verbosidad --------------------
using Str   = string;
using OS    = ostream;
template<typename T> using UP  = unique_ptr<T>;
template<typename T> using Vec = vector<T>;

// -------------------- Utilidad (posición y lectura) ----------------------
struct Loc { size_t line = 1, col = 1; };

static Str readFile(const Str& path){
    ifstream f(path, ios::binary);
    if(!f) throw runtime_error("No pude abrir archivo: " + path);
    ostringstream ss; ss << f.rdbuf(); return ss.str();
}

// ============================== LEXER =====================================
// Convierte texto fuente en un stream de tokens.
enum class Tok { 
    End, LBrack, RBrack, LPar, RPar, Comma, Eq, 
    Ident, Number, String, True, False,
    LBrace, RBrace, Colon, Null, Enum, Struct
};

static const char* tokName(Tok t){
    switch(t){
        case Tok::End:    return "End";
        case Tok::LBrack: return "[";
        case Tok::RBrack: return "]";
        case Tok::LPar:   return "(";
        case Tok::RPar:   return ")";
        case Tok::Comma:  return ",";
        case Tok::Eq:     return "=";
        case Tok::Ident:  return "IDENT";
        case Tok::Number: return "NUMBER";
        case Tok::String: return "STRING";
        case Tok::True:   return "TRUE";
        case Tok::False:  return "FALSE";case Tok::LBrace: return "{";
case Tok::RBrace: return "}";
case Tok::Colon:  return ":";
case Tok::Null:   return "NULL";
case Tok::Enum:   return "ENUM";
case Tok::Struct: return "STRUCT";

    }
    return "?";
}

struct Token { Tok t = Tok::End; Str lex; double num = 0; Loc loc; };

class Lexer {
    const Str s; size_t i = 0; Loc loc;

    static bool isIdStart(char c){ return isalpha((unsigned char)c) || c=='_'; }
    static bool isIdChar (char c){ return isalnum((unsigned char)c) || c=='_' || c=='.'; }
    char peek(int k=0) const { return (i+k < s.size() ? s[i+k] : '\0'); }
    char get() { char c=peek(); (c=='\n')? (loc.line++, loc.col=1) : (loc.col++); i++; return c; }

    // Ignora espacios y comentarios (# ... y // ...) hasta fin de línea
    void skipWSandComments(){
        while(true){
            // espacios
            while(isspace((unsigned char)peek())) get();
            // comentario '# ...'
            if (peek()=='#'){ while(peek() && peek()!='\n') get(); continue; }
            // comentario '// ...'
            if (peek()=='/' && peek(1)=='/'){ while(peek() && peek()!='\n') get(); continue; }
            break;
        }
    }

public:
    explicit Lexer(const Str& src): s(src) {}

    Token next(){
        skipWSandComments();
        Token k; k.loc = loc;
        if (i >= s.size()){ k.t = Tok::End; return k; }

        char c = peek();

        // Signos simples
        if (c=='['){ get(); k.t=Tok::LBrack; return k; }
        if (c==']'){ get(); k.t=Tok::RBrack; return k; }
        if (c=='('){ get(); k.t=Tok::LPar;   return k; }
        if (c==')'){ get(); k.t=Tok::RPar;   return k; }
        if (c==','){ get(); k.t=Tok::Comma;  return k; }
        if (c=='='){ get(); k.t=Tok::Eq;     return k; }
        if (c=='{'){ get(); k.t=Tok::LBrace; return k; }
if (c=='}'){ get(); k.t=Tok::RBrace; return k; }
if (c==':'){ get(); k.t=Tok::Colon;  return k; }

        // String con escapes básicos
        if (c=='"'){
            get(); Str out;
            while (peek() && peek()!='"'){
                char ch = get();
                if (ch=='\\' && peek()){
                    char e = get();
                    switch(e){ case 'n': out+='\n'; break; case 't': out+='\t'; break;
                               case 'r': out+='\r'; break; case '"': out+='"';  break;
                               case '\\':out+='\\'; break; default:  out+=e; }
                } else out += ch;
            }
            if (peek()=='"') get(); else throw runtime_error("String sin cerrar en linea " + to_string(loc.line));
            k.t = Tok::String; k.lex = out; return k;
        }

        // Número con signo opcional y decimal opcional
        if (isdigit((unsigned char)c) || (c=='-' && isdigit((unsigned char)peek(1)))){
            Str num; num.push_back(get()); bool hasDot=false;
            while (isdigit((unsigned char)peek())) num.push_back(get());
            if (peek()=='.'){ hasDot=true; num.push_back(get()); while(isdigit((unsigned char)peek())) num.push_back(get()); }
            k.t=Tok::Number; k.lex=num; k.num=strtod(num.c_str(), nullptr); return k;
        }

        // Ident o booleano (true/false). Permitimos '.' en el nombre
        if (isIdStart(c)){
    Str id; id.push_back(get());
    while (isIdChar(peek())) id.push_back(get());
    if (id=="true")   { k.t=Tok::True;  k.lex=id; return k; }
    if (id=="false")  { k.t=Tok::False; k.lex=id; return k; }
    if (id=="null")   { k.t=Tok::Null;  k.lex=id; return k; }
    if (id=="enum")   { k.t=Tok::Enum;  k.lex=id; return k; }
    if (id=="struct") { k.t=Tok::Struct;k.lex=id; return k; }
    k.t=Tok::Ident; k.lex=id; return k;
}

        // Carácter inesperado
        ostringstream msg; msg<<"Caracter inesperado '"<<c<<"' en linea "<<loc.line<<", col "<<loc.col;
        throw runtime_error(msg.str());
    }
};

// ================================ AST =====================================
// Nodos: Expr (valores), Stmt (asignaciones/sections), Program (raíz)
struct Node { Loc loc; virtual ~Node(){}; virtual void print(OS&, int) const = 0; };
static inline void pad(OS& os, int n){ while(n--) os<<' '; }

struct Expr : Node {};
struct ValNum   : Expr { double v; explicit ValNum(double x):v(x){};
    void print(OS& os,int n) const override { pad(os,n); os<<"(Number "<<v<<")\n"; } };
struct ValStr   : Expr { Str v;    explicit ValStr(Str s):v(move(s)){};
    void print(OS& os,int n) const override { pad(os,n); os<<"(String \""<<v<<"\")\n"; } };
struct ValBool  : Expr { 
    bool v;   
    explicit ValBool(bool b):v(b){}; 
    void print(OS& os,int n) const override { 
        pad(os,n); 
        os<<"(Bool "<<(v?"true":"false")<<")\n"; 
    } 
};
struct ValIdent : Expr { Str  v;   explicit ValIdent(Str s):v(move(s)){};
    void print(OS& os,int n) const override { pad(os,n); os<<"(Ident "<<v<<")\n"; } };
struct ValList  : Expr { Vec<UP<Expr>> xs;
    void print(OS& os,int n) const override { pad(os,n); os<<"(List\n"; for(auto& e: xs) e->print(os,n+2); pad(os,n); os<<")\n"; } };
struct ValTuple : Expr { Vec<UP<Expr>> xs;
    void print(OS& os,int n) const override { pad(os,n); os<<"(Tuple\n"; for(auto& e: xs) e->print(os,n+2); pad(os,n); os<<")\n"; } };

struct Stmt : Node {};
struct Assign  : Stmt { Str key; UP<Expr> val;
    void print(OS& os,int n) const override { pad(os,n); os<<"(Assign "<<key<<"\n"; if(val) val->print(os,n+2); pad(os,n); os<<")\n"; } };
struct Section : Stmt { Str name; Vec<UP<Stmt>> body;
    void print(OS& os,int n) const override { pad(os,n); os<<"(Section "<<name<<"\n"; for(auto& s: body) s->print(os,n+2); pad(os,n); os<<")\n"; } };
struct Program : Node { Vec<UP<Stmt>> items;
    void print(OS& os,int n) const override { pad(os,n); os<<"(Program\n"; for(auto& s: items) s->print(os,n+2); pad(os,n); os<<")\n"; } };
struct ValNull : Expr {
    void print(OS& os,int n) const override { pad(os,n); os<<"(Null)\n"; }
};

struct ValObject : Expr { Vec<pair<Str,UP<Expr>>> fields;
    void print(OS& os,int n) const override {
        pad(os,n); os<<"(Object\n";
        for(auto& kv: fields){
            pad(os,n+2); os<<"(Field "<<kv.first<<"\n";
            kv.second->print(os,n+4);
            pad(os,n+2); os<<")\n";
        }
        pad(os,n); os<<")\n";
    }
};

struct EnumDef : Stmt { Str name; Vec<pair<Str,double>> values;
    void print(OS& os,int n) const override {
        pad(os,n); os<<"(Enum "<<name<<"\n";
        for(auto& kv: values){
            pad(os,n+2); os<<"("<<kv.first<<" = "<<kv.second<<")\n";
        }
        pad(os,n); os<<")\n";
    }
};

struct StructDef : Stmt { Str name; Vec<Str> fields;
    void print(OS& os,int n) const override {
        pad(os,n); os<<"(Struct "<<name<<"\n";
        for(auto& f: fields){
            pad(os,n+2); os<<"(Field "<<f<<")\n";
        }
        pad(os,n); os<<")\n";
    }
};


// =============================== PARSER 1.0 ===================================
// Implementa gramática .brik:
// file    := (section | assign)* End
// section := '[' IDENT ']' assign*
// assign  := IDENT '=' expr
// expr    := NUMBER | STRING | TRUE | FALSE | IDENT | list | tuple
// list    := '[' (expr (',' expr)*)? ']'
// tuple   := '(' (expr (',' expr)*)? ')'
class Parser {
    Lexer L; Token cur;

    [[noreturn]] void expectFail(const char* what){
        ostringstream m; m<<"Se esperaba "<<what<<" en linea "<<cur.loc.line; throw runtime_error(m.str());
    }
    void eat(Tok t, const char* what){ if(cur.t!=t) expectFail(what); cur = L.next(); }

    // section := '[' IDENT ']' assign*
    UP<Stmt> parseSection(){
        eat(Tok::LBrack,"'['");
        if (cur.t != Tok::Ident) throw runtime_error("Nombre de seccion invalido en linea " + to_string(cur.loc.line));
        Str name = cur.lex; cur = L.next();
        eat(Tok::RBrack, "']'");
        auto sec = UP<Section>(new Section()); sec->name = move(name);
        while (cur.t == Tok::Ident) sec->body.emplace_back(parseAssign());
        return sec;
    }

    // assign := IDENT '=' expr
    UP<Stmt> parseAssign(){
        Str key = cur.lex; cur = L.next();
        eat(Tok::Eq, "'='");
        auto value = parseExpr();
        auto a = UP<Assign>(new Assign()); a->key = move(key); a->val = move(value); return a;
    }

    // expr := NUMBER | STRING | TRUE | FALSE | IDENT | list | tuple
    UP<Expr> parseExpr(){
        switch(cur.t){
            case Tok::Number: { double v = cur.num; cur = L.next(); return UP<Expr>(new ValNum(v)); }
            case Tok::String: { Str s = cur.lex;  cur = L.next(); return UP<Expr>(new ValStr(move(s))); }
            case Tok::True:   { cur = L.next();   return UP<Expr>(new ValBool(true)); }
            case Tok::False:  { cur = L.next();   return UP<Expr>(new ValBool(false)); }
            case Tok::Ident:  { Str s = cur.lex;  cur = L.next(); return UP<Expr>(new ValIdent(move(s))); }
            case Tok::LPar:   return parseTuple();
            case Tok::LBrack: return parseList();
            case Tok::Null:   { cur = L.next(); return UP<Expr>(new ValNull()); }
            case Tok::LBrace: return parseObject();

            default: { ostringstream m; m<<"Expresion invalida en linea "<<cur.loc.line; throw runtime_error(m.str()); }
        }
    }

    // tuple := '(' (expr (',' expr)*)? ')'
    UP<Expr> parseTuple(){
        eat(Tok::LPar,"'('");
        auto t = UP<ValTuple>(new ValTuple());
        if (cur.t != Tok::RPar){
            t->xs.emplace_back(parseExpr());
            while (cur.t == Tok::Comma){ cur = L.next(); t->xs.emplace_back(parseExpr()); }
        }
        eat(Tok::RPar,"')'"); return UP<Expr>(t.release());
    }

    // list := '[' (expr (',' expr)*)? ']'
    UP<Expr> parseList(){
        eat(Tok::LBrack,"'['");
        auto v = UP<ValList>(new ValList());
        if (cur.t != Tok::RBrack){
            v->xs.emplace_back(parseExpr());
            while (cur.t == Tok::Comma){ cur = L.next(); v->xs.emplace_back(parseExpr()); }
        }
        eat(Tok::RBrack, "']'"); return UP<Expr>(v.release());
    }

    // object := '{' (IDENT ':' expr (',' IDENT ':' expr)*)? '}'
    UP<Expr> parseObject(){
        eat(Tok::LBrace,"'{'");
        auto obj = UP<ValObject>(new ValObject());
        if (cur.t != Tok::RBrace){
            do {
                if (cur.t != Tok::Ident) expectFail("identificador de campo");
                Str key = cur.lex; cur = L.next();
                eat(Tok::Colon, "':'");
                auto val = parseExpr();
                obj->fields.emplace_back(move(key), move(val));
                if (cur.t == Tok::Comma) cur = L.next();
                else break;
            }
            while(true);
        }
        eat(Tok::RBrace,"'}'");
        return UP<Expr>(obj.release());
    }

    UP<Stmt> parseEnum(){
    eat(Tok::Enum,"'enum'");
    if (cur.t != Tok::Ident) expectFail("nombre de enum");
    Str name = cur.lex; cur = L.next();
    eat(Tok::LBrace,"'{'");
    auto e = UP<EnumDef>(new EnumDef()); e->name = move(name);
    while (cur.t == Tok::Ident){
        Str key = cur.lex; cur = L.next();
        eat(Tok::Colon, "':'");
        if (cur.t != Tok::Number) expectFail("valor numérico");
        double val = cur.num; cur = L.next();
        e->values.emplace_back(move(key), val);
        if (cur.t == Tok::Comma) cur = L.next(); else break;
    }
    eat(Tok::RBrace,"'}'");
    return e;
}

UP<Stmt> parseStruct(){
    eat(Tok::Struct,"'struct'");
    if (cur.t != Tok::Ident) expectFail("nombre de struct");
    Str name = cur.lex; cur = L.next();
    eat(Tok::LBrace,"'{'");
    auto s = UP<StructDef>(new StructDef()); s->name = move(name);
    while (cur.t == Tok::Ident){
        s->fields.push_back(cur.lex);
        cur = L.next();
        if (cur.t == Tok::Comma) cur = L.next();
        else if (cur.t == Tok::RBrace) break;
        else if (cur.t == Tok::Eq || cur.t == Tok::Colon){ 
            // permite "campo;" o "campo: valor"
            cur = L.next(); parseExpr();
            if (cur.t == Tok::Comma) cur = L.next();
        }
    }
    eat(Tok::RBrace,"'}'");
    return s;
}




public:
    explicit Parser(const Str& src): L(src) { cur = L.next(); }

    // file := (section | assign | enum | struct)* End
    UP<Program> parse(){
        auto p = UP<Program>(new Program());
        while (cur.t != Tok::End){
            if      (cur.t == Tok::LBrack)  p->items.emplace_back(parseSection());
            else if (cur.t == Tok::Ident)   p->items.emplace_back(parseAssign());
            else if (cur.t == Tok::Enum)    p->items.emplace_back(parseEnum());
            else if (cur.t == Tok::Struct)  p->items.emplace_back(parseStruct());
            else { ostringstream m; m<<"Token inesperado en linea "<<cur.loc.line; throw runtime_error(m.str()); }
        }
        return p;
    }
};

// =========================== Dumper de Tokens ================================
static void dumpTokens(const Str& src, ostream& os){
    Lexer L(src);
    Token t;
    do {
        t = L.next();
        os << tokName(t.t);
        if(!t.lex.empty()) os << "('" << t.lex << "')";
        os << " @line " << t.loc.line << "\n";
    } while(t.t != Tok::End);
}

// =========================== Printer + CLI ================================
static void writeAST(const Program& p, const Str& out){
    ofstream o(out, ios::binary);
    if(!o) throw runtime_error("No pude abrir salida: " + out);
    p.print(o, 0);
}
static void usage(const char* argv0){
    cerr<<"Uso: "<<argv0<<" <archivo.brik> [-o arbol.ast] [--tokens] [--tokfile tokens.txt] [--no-ast]\n";
}

int main(int argc, char** argv){
    if (argc < 2){ usage(argv[0]); return 1; }
    Str in = argv[1], out = "arbol.ast", tokOut = "";
    bool wantTokens = false, wantAST = true;

    // Parseo de flags
    for (int i=2;i<argc;i++){
        Str a = argv[i];
        if      (a=="-o" && i+1<argc)      out = argv[++i];
        else if (a=="--tokens")            wantTokens = true;
        else if (a=="--tokfile" && i+1<argc) tokOut = argv[++i];
        else if (a=="--no-ast")            wantAST = false;
        else { cerr<<"Argumento no reconocido: "<<a<<"\n"; usage(argv[0]); return 2; }
    }

    try {
        Str src = readFile(in);

        // 1) Tokenizar si se pidió
        if (wantTokens){
            if (tokOut.empty()) dumpTokens(src, cout);
            else { ofstream tf(tokOut, ios::binary); if(!tf) throw runtime_error("No pude abrir "+tokOut); dumpTokens(src, tf); }
        }

        // 2) Parsear y escribir AST (por defecto)
        if (wantAST){
            Parser P(src);
            auto prog = P.parse();
            writeAST(*prog, out);
            cout<<"OK. AST escrito en: "<<out<<"\n";
        }
        return 0;
    } catch(const exception& e){
        cerr<<"Error: "<<e.what()<<"\n"; return 3;
    }
}
