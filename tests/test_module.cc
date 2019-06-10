#include "../src/module.hh"
#include "gtest/gtest.h"

TEST(module, load) {    // NOLINT
    auto mod = Module::from_verilog("module1.sv", "module1", {}, {});
    EXPECT_TRUE(mod.ports.find("f") != mod.ports.end());
    ASSERT_ANY_THROW(Module::from_verilog("module1.sv", "module3", {}, {}));
    ASSERT_ANY_THROW(Module::from_verilog("module1.sv", "module1", {"NON_EXIST"}, {}));
    mod = Module::from_verilog("module1.sv", "module1", {}, {{"a", PortType::Clock}});
    EXPECT_EQ(mod.ports.at("a").type, PortType::Clock);
    ASSERT_ANY_THROW(Module::from_verilog("module1.sv", "module1", {}, {{"aa", PortType::Clock}}));
}

TEST(module, port) {    // NOLINT
    auto mod = Module("module");
    Port p_in(PortDirection::In, "in", 1);
    Port p_out(PortDirection::Out, "out", 1);
    mod.add_port(p_in);
    mod.add_port(p_out);
}

TEST(module, expr) {    // NOLINT
    auto mod = Module("module");
    Port p_in(PortDirection::In, "in", 1);
    Port p_out(PortDirection::Out, "out", 1);

    Var &var1 = mod.var("a", 1);
    Var var2 = mod.var("b", 1);
    auto expr = var1 + var2;
    EXPECT_EQ(expr.left, &var1);
    // var2 is stored in stack
    EXPECT_NE(expr.right, &var2);

    expr = (var1 - var2).ashr(var2);
    EXPECT_EQ(expr.name, "a_sub_b_ashr_b");
}