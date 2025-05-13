#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <optional>

enum class TokenType
{
    INTEGER_LITERAL,
    OPERATOR_NEG,
    OPERATOR_BIT_NOT,
    OPERATOR_LOG_NOT,
    SEMICOLON,
    END_OF_FILE,
    UNKNOWN
};

struct Token
{
    TokenType type;
    std::string value;
    int line = 1;
    int col = 1;

    std::string type_to_string() const
    {
        switch (type)
        {
        case TokenType::INTEGER_LITERAL:
            return "INTEGER_LITERAL";
        case TokenType::OPERATOR_NEG:
            return "OPERATOR_NEG";
        case TokenType::OPERATOR_BIT_NOT:
            return "OPERATOR_BIT_NOT";
        case TokenType::OPERATOR_LOG_NOT:
            return "OPERATOR_LOG_NOT";
        case TokenType::SEMICOLON:
            return "SEMICOLON";
        case TokenType::END_OF_FILE:
            return "END_OF_FILE";
        case TokenType::UNKNOWN:
            return "UNKNOWN";
        default:
            return "INVALID_TOKEN_TYPE";
        }
    }
};

class Lexer
{
private:
    std::string source;
    size_t current_pos = 0;
    int current_line = 1;
    int current_col = 1;

    char advance()
    {
        if (current_pos >= source.length())
        {
            return '\0';
        }
        char current_char = source[current_pos];
        current_pos++;
        if (current_char == '\n')
        {
            current_line++;
            current_col = 1;
        }
        else
        {
            current_col++;
        }
        return current_char;
    }

    char peek() const
    {
        if (current_pos >= source.length())
        {
            return '\0';
        }
        return source[current_pos];
    }

    void skip_whitespace()
    {
        while (current_pos < source.length() && std::isspace(peek()))
        {
            advance();
        }
    }

    Token read_integer()
    {
        size_t start_pos = current_pos - 1;
        int start_col = current_col - 1;
        while (std::isdigit(peek()))
        {
            advance();
        }
        std::string number_str = source.substr(start_pos, current_pos - start_pos);
        return {TokenType::INTEGER_LITERAL, number_str, current_line, start_col};
    }

public:
    Lexer(const std::string &src) : source(src) {}

    Token next_token()
    {
        skip_whitespace();

        if (current_pos >= source.length())
        {
            return {TokenType::END_OF_FILE, "", current_line, current_col};
        }

        char current_char = advance();
        int start_col = current_col - 1;

        if (std::isdigit(current_char))
        {
            return read_integer();
        }

        switch (current_char)
        {
        case '-':
            return {TokenType::OPERATOR_NEG, "-", current_line, start_col};
        case '~':
            return {TokenType::OPERATOR_BIT_NOT, "~", current_line, start_col};
        case '!':
            return {TokenType::OPERATOR_LOG_NOT, "!", current_line, start_col};
        case ';':
            return {TokenType::SEMICOLON, ";", current_line, start_col};
        default:
            std::cerr << "Warning: Unknown character '" << current_char
                      << "' at line " << current_line << ", col " << start_col << std::endl;
            return {TokenType::UNKNOWN, std::string(1, current_char), current_line, start_col};
        }
    }

    std::vector<Token> tokenize_all()
    {
        std::vector<Token> tokens;
        Token token;
        do
        {
            token = next_token();
            tokens.push_back(token);
        } while (token.type != TokenType::END_OF_FILE);
        return tokens;
    }
};

class Parser
{
private:
    std::vector<Token> tokens;
    size_t current_token_index = 0;
    std::vector<std::string> assembly_output;

    const Token &current_token() const
    {
        if (current_token_index >= tokens.size())
        {
            return tokens.back();
        }
        return tokens[current_token_index];
    }

    const Token &consume_token()
    {
        if (current_token_index >= tokens.size())
        {
            throw std::runtime_error("Attempted to consume past end of tokens.");
        }
        return tokens[current_token_index++];
    }

    const Token &expect_token(TokenType expected_type)
    {
        const Token &token = current_token();
        if (token.type != expected_type)
        {
            throw std::runtime_error("Syntax Error at line " + std::to_string(token.line) +
                                     ", col " + std::to_string(token.col) +
                                     ": Expected token type " + Token{expected_type}.type_to_string() +
                                     " but got " + token.type_to_string());
        }
        return consume_token();
    }

    void parse_factor()
    {
        const Token &token = current_token();

        switch (token.type)
        {
        case TokenType::INTEGER_LITERAL:
            assembly_output.push_back("  PUSH " + token.value);
            consume_token();
            break;
        case TokenType::OPERATOR_NEG:
        case TokenType::OPERATOR_BIT_NOT:
        case TokenType::OPERATOR_LOG_NOT:
        {
            TokenType op_type = token.type;
            consume_token();
            const Token &operand_token = expect_token(TokenType::INTEGER_LITERAL);
            assembly_output.push_back("  PUSH " + operand_token.value);

            if (op_type == TokenType::OPERATOR_NEG)
            {
                assembly_output.push_back("  NEG");
            }
            else if (op_type == TokenType::OPERATOR_BIT_NOT)
            {
                assembly_output.push_back("  NOT");
            }
            else
            {
                assembly_output.push_back("  LNOT");
            }
        }
        break;
        default:
            throw std::runtime_error("Syntax Error at line " + std::to_string(token.line) +
                                     ", col " + std::to_string(token.col) +
                                     ": Expected integer or unary operator, but got " +
                                     token.type_to_string());
        }
    }

    void parse_statement()
    {
        parse_factor();
        expect_token(TokenType::SEMICOLON);
        // Removed assembly comment here too
    }

public:
    Parser(const std::vector<Token> &toks) : tokens(toks)
    {
        if (tokens.empty() || tokens.back().type != TokenType::END_OF_FILE)
        {
            throw std::invalid_argument("Token list must end with EOF token.");
        }
    }

    void parse()
    {
        assembly_output.push_back("section .text");
        assembly_output.push_back("global _start");
        assembly_output.push_back("_start:");

        while (current_token().type != TokenType::END_OF_FILE)
        {
            try
            {
                parse_statement();
            }
            catch (const std::runtime_error &e)
            {
                std::cerr << "Error during parsing: " << e.what() << std::endl;
                while (current_token_index < tokens.size() &&
                       current_token().type != TokenType::SEMICOLON &&
                       current_token().type != TokenType::END_OF_FILE)
                {
                    consume_token();
                }
                if (current_token().type == TokenType::SEMICOLON)
                {
                    consume_token();
                }
                if (current_token().type == TokenType::END_OF_FILE)
                    break;
            }
        }

        assembly_output.push_back("");
        assembly_output.push_back("  MOV RAX, 60");
        assembly_output.push_back("  XOR RDI, RDI");
        assembly_output.push_back("  SYSCALL");
    }

    const std::vector<std::string> &get_assembly() const
    {
        return assembly_output;
    }
};

int main()
{
    std::string source_code = R"(
        42;
        -123;
        ~5;
        !0;
        - 99 ;
        ! 1 ;
        ~ -2 ;
        abc;
        100
    )";

    std::cout << "--- Source Code ---" << std::endl;
    std::cout << source_code << std::endl;

    std::cout << "--- Lexing ---" << std::endl;
    Lexer lexer(source_code);
    std::vector<Token> tokens = lexer.tokenize_all();

    std::cout << "Tokens found:" << std::endl;
    for (const auto &token : tokens)
    {
        std::cout << "  Type: " << token.type_to_string()
                  << ", Value: '" << token.value << "'"
                  << " (L:" << token.line << ", C:" << token.col << ")" << std::endl;
        if (token.type == TokenType::UNKNOWN)
        {
            std::cerr << "Lexer Warning: Encountered unknown token." << std::endl;
        }
    }
    std::cout << std::endl;

    std::cout << "--- Parsing & Code Generation ---" << std::endl;
    Parser parser(tokens);
    try
    {
        parser.parse();
        const auto &assembly = parser.get_assembly();
        std::cout << "Generated Pseudo-Assembly:" << std::endl;
        for (const auto &line : assembly)
        {
            std::cout << line << std::endl;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "Parser Error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::invalid_argument &e)
    {
        std::cerr << "Initialization Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
