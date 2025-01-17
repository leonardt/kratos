Kratos
======

|Build Status| |PyPI - Format| |PyPI - Version| |Documentation Status|

Kratos is a hardware design language written in C++/Python. It
differentiates itself from other DSL with the following design
philosophy:

- Fully debuggable: users can see a trace of every passes on
  every verilog statement.
- Highly efficient: Python frontend powered by
  Modern C++ binding. Designed with multi-processing in mind.
- Human-readable verilog: we know how difficult it is to read machine
  generated verilog. kratos has multiple passes to produce nice-looking
  verilog.
- Generator of generators: every python object is a generator
  that can be modified at any time, even after instantiation. This allows
  complex passes on the generators without ripping old structure apart.
- Keep the good parts of verilog: The ``always`` block in behavioral
  verilog is close to other programming languages. Kratos allows you to
  write python code similar to behavioral verilog
- Single source of truth: kratos encourages users to infuse generator
  information inside generator itself. This makes debugging and
  verification much easier.
- Static elaboration: kratos allows user to write parametrized code,
  even in the ``always`` block, all in Python.
- Type checking: kratos check the variable types
  for each assignment to make sure there is no implicit conversion.

Install
-------

::

   pip install kratos

To build it from scratch, you need a ``C++17`` compatible compiler, such
as ``g++-8``.

Documentation and Examples
--------------------------

You can check the documentation at `Read the
Docs <https://kratos-doc.readthedocs.io/en/latest/>`__

Here are some examples to showcase the ability of kratos.

Asnyc Reset Register
~~~~~~~~~~~~~~~~~~~~

Python code that parametrizes based on the ``width``. Notice that we
specify the sensitivity of the ``always`` block when defining
``seq_code_block``.

.. code:: python

   class AsyncReg(Generator):
       def __init__(self, width):
           super().__init__("register")

           # define inputs and outputs
           self._in = self.port("in", width, PortDirection.In)
           self._out = self.port("out", width, PortDirection.Out)
           self._clk = self.port("clk", 1, PortDirection.In, PortType.Clock)
           self._rst = self.port("rst", 1, PortDirection.In,
                                 PortType.AsyncReset)
           self._val = self.var("val", width)

           # add combination and sequential blocks
           self.add_code(self.seq_code_block)

           self.add_code(self.comb_code_block)

       @always([(BlockEdgeType.Posedge, "clk"),
                (BlockEdgeType.Posedge, "rst")])
       def seq_code_block(self):
           if self._rst:
               self._val = 0
           else:
               self._val = self._in

       def comb_code_block(self):
           self._out = self._val

Here is the generated verilog

.. code:: verilog

   module register (
     input  clk,
     input [15:0] in,
     output reg [15:0] out,
     input  rst
   );

   logic  [15:0] val;

   always @(posedge clk, posedge rst) begin
     if rst begin
       val <= 16'h0;
     end
     else begin
       val <= in;
     end
   end
   always_comb begin
     out = val;
   end
   endmodule   // register

Fanout module
~~~~~~~~~~~~~

This is an example to showcase the kratos’ static elaboration ability in
``always`` block. In practice we would not write it this way.

.. code:: python

   class PassThrough(Generator):
       def __init__(self, num_loop):
           super().__init__("PassThrough", True)
           self.in_ = self.port("in", 1, PortDirection.In)
           self.out_ = self.port("out", num_loop, PortDirection.Out)
           self.num_loop = num_loop

           self.add_code(self.code)

       def code(self):
           if self.in_ == self.const(1, 1):
               for i in range(self.num_loop):
                   self.out_[i] = 1
           else:
               for i in range(self.num_loop):
                   self.out_[i] = 0

Here is generated verilog

.. code:: verilog

   module PassThrough (
     input  in,
     output reg [3:0] out
   );

   always_comb begin
     if (in == 1'h1) begin
       out[0:0] = 1'h1;
       out[1:1] = 1'h1;
       out[2:2] = 1'h1;
       out[3:3] = 1'h1;
     end
     else begin
       out[0:0] = 1'h0;
       out[1:1] = 1'h0;
       out[2:2] = 1'h0;
       out[3:3] = 1'h0;
     end
   end
   endmodule   // PassThrough

How to debug
------------

Because Python is quite slow, By default the debug option is off. You
can turn on debugging for individual modules. Here is an example on how
to turn on debug (see ``tests/test_generator.py`` for more details).

.. code:: python

   class PassThroughMod(Generator):
       def __init__(self):
           super().__init__("mod1", True)
           self.in_ = self.port("in", 1, PortDirection.In)
           self.out_ = self.port("out", 1, PortDirection.Out)
           self.wire(self.out_, self.in_)

   # ... some other code
   class Top(Generator):
       def __init__(self):
           super().__init__("top", True)

           self.port("in", 1, PortDirection.In)
           self.port("out", 1, PortDirection.Out)

           pass_through = PassThroughMod()
           self.add_child_generator("pass", pass_through)
           self.wire(self["pass"].ports["in"], self.ports["in"])

           self.wire(self.ports.out, self["pass"].ports.out)

   mod = Top()
   mod_src, debug_info = verilog(mod, debug=True)

You can see the generated verilog:

.. code:: verilog

   module top (
     input logic  in,
     output logic  out
   );

   assign out = in;
   endmodule   // top

The ``pass`` sub-module disappeared due to the compiler optimization.
However, if we print out the debug information, we can see the full
trace of debug info on ``assign out = in;``

.. code:: python

   {
     1: [('/home/keyi/workspace/kratos/tests/test_generator.py', 532)],
     2: [('/home/keyi/workspace/kratos/tests/test_generator.py', 534)],
     3: [('/home/keyi/workspace/kratos/tests/test_generator.py', 535)],
     6: [('/home/keyi/workspace/kratos/tests/test_generator.py', 539),
         ('/home/keyi/workspace/kratos/src/expr.cc', 455),
         ('/home/keyi/workspace/kratos/tests/test_generator.py', 541),
         ('/home/keyi/workspace/kratos/src/expr.cc', 485),
         ('/home/keyi/workspace/kratos/src/pass.cc', 653)]
   }

These ``pass.cc`` is the pass that removed the pass through module.

If we modified the source code a little bit that change the wire
assignment into a combination block, such as

.. code:: python

   class Top(Generator):
       def __init__(self):
           super().__init__("top", True)

           self.port("in", 1, PortDirection.In)
           self.port("out", 1, PortDirection.Out)

           pass_through = PassThroughMod()
           self.add_child_generator("pass", pass_through)
           self.wire(self["pass"].ports["in"], self.ports["in"])

           self.add_code(self.code_block)

       def code_block(self):
           self.ports.out = self["pass"].ports.out

We can see the generated verilog will be a little bit verbose:

.. code:: verilog

   module top (
     input logic  in,
     output logic  out
   );

   logic   top$in_0;
   assign top$in_0 = in;
   always_comb begin
     out = top$in_0;
   end
   endmodule   // top

And the debug info shows all the information as well:

.. code:: python

   {
     1: [('/home/keyi/workspace/kratos/tests/test_generator.py', 554)],
     2: [('/home/keyi/workspace/kratos/tests/test_generator.py', 556)],
     3: [('/home/keyi/workspace/kratos/tests/test_generator.py', 557)],
     7: [('/home/keyi/workspace/kratos/tests/test_generator.py', 561), ('/home/keyi/workspace/kratos/src/expr.cc', 455)],
     8: [('/home/keyi/workspace/kratos/tests/test_generator.py', 563)],
     9: [('/home/keyi/workspace/kratos/tests/test_generator.py', 566), ('/home/keyi/workspace/kratos/src/expr.cc', 485)]}

Ecosystem
---------

Similar to `Magma <https://github.com/phanrahan/magma>`__, kratos has
its own ecosystem to program behavioral verilog in Python. They are
named after sons of Titans in Greek mythology.

`kratos <https://github.com/Kuree/kratos>`__ is a programming model for
building hardware. The main abstraction in kratos in a ``Generator``.
``Generator`` can be modified at any time through passes.

`zelus <https://github.com/Kuree/zelus>`__ is a library of useful
generators, such as mux and decoder. They are designed to be as
efficient as possible.


.. |Build Status| image:: https://travis-ci.com/Kuree/kratos.svg?branch=master
   :target: https://travis-ci.com/Kuree/kratos
.. |PyPI - Format| image:: https://img.shields.io/pypi/format/kratos.svg
   :target: https://pypi.org/project/kratos/
.. |PyPI - Version| image:: https://badge.fury.io/py/kratos.svg
   :target: https://pypi.org/project/kratos/
.. |Documentation Status| image:: https://readthedocs.org/projects/kratos-doc/badge/?version=latest
   :target: https://kratos-doc.readthedocs.io/en/latest/?badge=latest
