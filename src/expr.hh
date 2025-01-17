#ifndef KRATOS_EXPR_HH
#define KRATOS_EXPR_HH

#include <set>
#include <string>
#include <unordered_set>
#include <vector>
#include "ast.hh"
#include "context.hh"

enum ExprOp : uint64_t {
    // unary
    UInvert,
    UMinus,
    UPlus,

    // binary
    Add,
    Minus,
    Divide,
    Multiply,
    Mod,
    LogicalShiftRight,
    SignedShiftRight,
    ShiftLeft,
    Or,
    And,
    Xor,

    // relational
    LessThan,
    GreaterThan,
    LessEqThan,
    GreaterEqThan,
    Eq,
    Neq
};

bool is_relational_op(ExprOp op);

enum VarType { Base, Expression, Slice, ConstValue, PortIO, Parameter, BaseCasted };

enum VarCastType { Signed, Clock, AsyncReset};

struct Var : public std::enable_shared_from_this<Var>, public ASTNode {
public:
    Var(Generator *m, const std::string &name, uint32_t width, bool is_signed);
    Var(Generator *m, const std::string &name, uint32_t width, bool is_signed, VarType type);

    std::string name;
    uint32_t width;
    bool is_signed;

    // overload all the operators
    // unary
    Expr &operator~() const;
    Expr &operator-() const;
    Expr &operator+() const;
    // binary
    Expr &operator+(const Var &var) const;
    Expr &operator-(const Var &var) const;
    Expr &operator*(const Var &var) const;
    Expr &operator%(const Var &var) const;
    Expr &operator/(const Var &var) const;
    Expr &operator>>(const Var &var) const;
    Expr &operator<<(const Var &var) const;
    Expr &operator|(const Var &var) const;
    Expr &operator&(const Var &var) const;
    Expr &operator^(const Var &var) const;
    Expr &ashr(const Var &var) const;
    Expr &operator<(const Var &var) const;
    Expr &operator>(const Var &var) const;
    Expr &operator<=(const Var &var) const;
    Expr &operator>=(const Var &var) const;
    Expr &operator!=(const Var &var) const;
    Expr &eq(const Var &var) const;
    // slice
    virtual VarSlice &operator[](std::pair<uint32_t, uint32_t> slice);
    virtual VarSlice &operator[](uint32_t bit);
    // concat
    virtual VarConcat &concat(Var &var);
    void add_concat_var(const std::shared_ptr<VarConcat> &var) { concat_vars_.emplace(var); }

    std::shared_ptr<Var> cast(VarCastType cast_type);

    // assignment
    AssignStmt &assign(const std::shared_ptr<Var> &var);
    AssignStmt &assign(Var &var);
    virtual AssignStmt &assign(const std::shared_ptr<Var> &var, AssignmentType type);
    AssignStmt &assign(Var &var, AssignmentType type);
    void unassign(const std::shared_ptr<AssignStmt> &stmt);

    Generator *generator;
    ASTNode *parent() override;

    VarType type() const { return type_; }
    const std::unordered_set<std::shared_ptr<AssignStmt>> &sinks() const { return sinks_; };
    void remove_sink(const std::shared_ptr<AssignStmt> &stmt) { sinks_.erase(stmt); }
    const std::unordered_set<std::shared_ptr<AssignStmt>> &sources() const { return sources_; };
    void remove_source(const std::shared_ptr<AssignStmt> &stmt) { sources_.erase(stmt); }
    std::map<std::pair<uint32_t, uint32_t>, std::shared_ptr<VarSlice>> &get_slices() {
        return slices_;
    }

    static void move_src_to(Var *var, Var *new_var, Generator *parent, bool keep_connection);
    static void move_sink_to(Var *var, Var *new_var, Generator *parent, bool keep_connection);
    virtual void add_sink(const std::shared_ptr<AssignStmt> &stmt) { sinks_.emplace(stmt); }
    virtual void add_source(const std::shared_ptr<AssignStmt> &stmt) { sources_.emplace(stmt); }

    template <typename T>
    std::shared_ptr<T> as() {
        return std::static_pointer_cast<T>(shared_from_this());
    }

    virtual std::string to_string() const;

    // AST stuff
    void accept(ASTVisitor *visitor) override { visitor->visit(this); }
    uint64_t child_count() override { return 0; }
    ASTNode *get_child(uint64_t) override { return nullptr; }

    Var(const Var& var) = delete;
    Var() = delete;

protected:
    std::unordered_set<std::shared_ptr<AssignStmt>> sinks_;
    std::unordered_set<std::shared_ptr<AssignStmt>> sources_;

    VarType type_ = VarType::Base;

    std::unordered_set<std::shared_ptr<VarConcat>> concat_vars_;

private:
    std::pair<std::shared_ptr<Var>, std::shared_ptr<Var>> get_binary_var_ptr(const Var &var) const;
    std::map<std::pair<uint32_t, uint32_t>, std::shared_ptr<VarSlice>> slices_;

    std::unordered_map<VarCastType, std::shared_ptr<VarCasted>> casted_;
};

struct VarCasted : public Var {
public:
    VarCasted(Var *parent, VarCastType cast_type);
    AssignStmt &assign(const std::shared_ptr<Var> &var, AssignmentType type) override;

    void add_sink(const std::shared_ptr<AssignStmt> &stmt) override;

    std::string to_string() const override;

private:
    Var *parent_var_ = nullptr;

    VarCastType cast_type_;
};

struct VarSlice : public Var {
public:
    Var *parent_var = nullptr;
    uint32_t low = 0;
    uint32_t high = 0;

    VarSlice(Var *parent, uint32_t high, uint32_t low);
    ASTNode *parent() override;

    // we tie it to the parent
    void add_sink(const std::shared_ptr<AssignStmt> &stmt) override;
    void add_source(const std::shared_ptr<AssignStmt> &stmt) override;

    void set_parent(Var* parent) { parent_var = parent; }

    void accept(ASTVisitor *visitor) override { visitor->visit(this); }

    static std::string get_slice_name(const std::string &parent_name, uint32_t high, uint32_t low);

    std::string to_string() const override;
};

struct VarConcat : public Var {
public:
    std::vector<std::shared_ptr<Var>> vars;
    VarConcat(Generator *m, const std::shared_ptr<Var> &first, const std::shared_ptr<Var> &second);
    VarConcat(const VarConcat &var);

    VarConcat &concat(Var &var) override;
    void accept(ASTVisitor *visitor) override { visitor->visit(this); }

    std::string to_string() const override;
};

struct Const : public Var {
    // need to rewrite the const backend since the biggest number is uint64_t, which may not
public:
    Const(Generator *m, int64_t value, uint32_t width, bool is_signed);

    int64_t value() { return value_; }
    void set_value(int64_t new_value);
    void add_source(const std::shared_ptr<AssignStmt> &stmt) override;

    std::string to_string() const override;

    void accept(ASTVisitor *visitor) override { visitor->visit(this); }

private:
    int64_t value_;
};

struct Param: public Const {
public:
    Param(Generator *m, std::string name, uint32_t width, bool is_signed);

    void accept(ASTVisitor *visitor) override { visitor->visit(this); }

    std::string inline to_string() const override { return parameter_name_; }

    std::string inline value_str() const { return Const::to_string(); }

private:
    std::string parameter_name_;

};

struct Expr : public Var {
    ExprOp op;
    std::shared_ptr<Var> left;
    std::shared_ptr<Var> right;

    Expr(ExprOp op, const std::shared_ptr<Var> &left, const std::shared_ptr<Var> &right);
    std::string to_string() const override;
    void add_sink(const std::shared_ptr<AssignStmt> &stmt) override;

    // AST
    void accept(ASTVisitor *visitor) override { visitor->visit(this); }
    uint64_t child_count() override { return right ? 2 : 1; }
    ASTNode *get_child(uint64_t index) override;
};

#endif  // KRATOS_EXPR_HH
