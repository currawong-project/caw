# __caw__ By Example

__caw__ is a configuration language for describing data flow based
real-time audio signal processing programs. The language
is based on the __flow__ framework within [libcw](https://gitea.currawongproject.org/cml/libcw).

Features of the language include:

- Support for low-latency, interactive, real-time as well as non real-time programs.
- Easily described distributed, parallel processing.
- Clear and easily interpreted network execution semantics.
- Automatic user interface generation.
- Large collection of data flow unit processors.
- Automatic multi-thread programming support.




### Example 01 - Write a sine signal to an audio file.

__caw__ programs are described using a slightly extended form of JSON. 
In this example the program is contained in the dictionary labeled `sine_file_01` and
the preceeding fields (e.g. `base_dir`,`proc_dict`,`subnet_dict`, etc.) contain
system parameters that the program needs to compile and run the program.

``` javascript
{
  base_dir:    "~/src/caw/examples/io",
  proc_dict:   "~/src/caw/examples/proc_dict.cfg",
  mode: non_real_time,
    
  programs: {

    ex_01_sine_file: {

      dur_limit_secs:5.0,  // Run the network for 5 seconds

      network: {

        procs: {
          // Create a 'sine_tone' oscillator.
          osc: { class: sine_tone },
          
          // Create an audio output file and fill it with the output of the oscillator.
          af:  { class: audio_file_out, in: { in:osc.out } args:{  fname:"$/out.wav"} }
        } 
      }      
    }
  }
}
```

![Example 0](svg/00_osc_af.svg "`ex_01_sine_file` processing network")

When executed this program will write a five second sine signal to an audio file
named `~/src/caw/examples/sine_file_01/out.wav`.  The output file name
is formed by joining the value of the system parameter `base_dir` with 
the name of the program `sine_file_01`.

Run the program like this:
```
caw example.cfg sine_file_01
```

__caw__ programs specify and run a network of virtual processors.  The network is
described in the `procs` dictionary. 

The line `osc: { ... }` defines an instance of a `sine_tone` processor
named `osc`. The line `af: { ... }` defines an instance of a `audio_file_out` processor
named `af`.

In the language of __caw__ `osc` and `af`  are refered to as _processor instances_ or
sometimes just _processors_.

`osc` and `af` are connected together using the `in:{ ... }` statement in the `af` 
instance description.  The `in` statement connects `osc.out` to `af.in` and
thereby directs the output of the signal generator into the audio file.

The `args:{ ... }` statment lists processor specific arguments used to create the 
`af` instance. In this case `af.fname` names the output file. The use of the
`$` prefix on the file name indicates that the file should be written to 
the _project directory_ which is formed by joining `base_dir` with the program name.
The _project directory_ is automatically created, if it doesn't already exist, when the program is run.


### Processor Class Descriptions

- _processors_ are collections of named __variables__ which are defined in the
processor class file named by the `proc_dict` system parameter field.

Here are the class specifications for `sine_tone` and `audio_file_out`.

```yaml
sine_tone: {
  vars: {
      srate:     { type:srate, value:0,         doc:"Sine tone sample rate. 0=Use default system sample rate"}
      ch_cnt:    { type:uint,  value:2,         doc:"Output signal channel count."},
      hz:        { type:coeff, value:440.0,     doc:"Frequency in Hertz."},
      phase:     { type:coeff, value:0.0,       doc:"Offset phase in radians."},
      dc:        { type:coeff, value:0.0,       doc:"DC offset applied after gain."},
      gain:      { type:coeff, value:0.8,       doc:"Signal frequency."},
      out:       { type:audio, flags['no_src'], doc:"Audio output" },
  }

  presets: {
      a220 : { hz:220 },
      a440 : { hz:440 },
      a880 : { hz:880 },
      mono:  { ch_cnt:1, gain:0.75 }
  }
}

audio_file_out: {
  vars: {
    fname: { type:string,               doc:"Audio file name." },
    bits:  { type:uint, value:32u,      doc:"Audio file word width. (8,16,24,32,0=float32)."},
    in:    { type:audio, flags:["src"], doc:"Audio file input." }
  }
}
```

The class definitions specify the names, types and default values for
each variable.  Since the `sine_tone` instance in `ex_01_sine_file`
doesn't override any of the the variable default values the generated
audio file must be a stereo (`ch_cnt`=2), 440 Hertz signal with an
amplitude of 0.8.

Note that unless stated otherwise all variables can be either input or
output ports for their processor. The `no_src` attribute on
`sine_tone.out` indicates that it is an output-only variable. The
`src` attribute on `audio_file_out.in` indicates that it must be
connected to a source variable or the processor cannot be instantiated -
and therefore the network it is contained by cannot be instantiated.
Note that this isn't to say that it can't be an output variable - only
that it must be connected.


Here is a complete list of possible variable attributes.
Attribute | Description
----------|-------------------------------------------------------
src       | This variable must be connected to a source variable or the processor instantiation will fail.
no_src    | This variable cannot be connected to a source variable (it is write-only, or output only).
init      | This variable is only read at processer instantiation time, changes during runtime will be ignored.
mult      | This variable may be instantiated multiple times. See `ex_05_mult_input` below.
out       | This is a subnet output variable. 

__caw__ uses types and does it's best at converting between types where the conversion will
not lose information.

Here are the list of built-in types:

Type     | Description
---------|-----------------------------------
bool     | true | false
uint     | C unsigned
int      | C int
float    | C float
double   | C double
string   | Array of bytes.
time     | POSIX timespec
cfg      | cw object (JSON object)
audio    | multi-channel audio array
spectrum | multi-channel spectrum in comlex or rect. coordinates.
midi     | MIDI message array.
runtime  | 'no_src' variable whose type is determined by the types of the other variables. See the 'list' processor.
numeric  | bool | uint | int | float | double
all      | This variable can be any type. Commonly used for variables which act as triggers. See the 'counter' processor.
record   | This type is used to form records of fields based on the the above types.

A few type aliases are also defined. Thse aliases are intended to help document the intended purpose of a given variable.

Type aliases:
Alias    | Type   | Description
---------|--------|----------------------------
srate    | float  | This is an audio sample rate value.
sample   | float  | This value is calculated from audio sample values (e.g. RMS )
coeff    | float  | This value will operate (e.g. add, multiply) on an audio signal.
ftime    | double | Fractional time in seconds or milliseconds.

Also notice that the processor class has named presets. During
processor instantiaion these presets can be used to set the
initial state of the processor.  See `ex_02_mod_sine` below for
an example of a class preset used this way.


### Example 02: Modulated Sine Signal

This example extends `ex_01_sine_file` by using a low frequency oscillator (LFO) to
modulate the frequency of a second audio oscillator.  The LFO 
is formed using a second `sine_tone` processor and a sample and hold unit. The output
of the sample and hold unit is then used to set the frequency of the audio
`sine_tone` oscillator.

Note that the LFO output is a 3 Hertz sine signal
with a gain of 110 (220 peak-to-peak amplitude) and an offset
of 440.  The LFO output signal is therefore sweeping an amplitude
between 330 and 550 which will be treated as frequency values by `osc`.

``` json
ex_02_mod_sine: {

  dur_limit_secs:5.0,

  network: {

    procs: {
      lfo:   { class: sine_tone, args:{ hz:3, dc:440, gain:110 }}
      sh:    { class: sample_hold,              in:{ in:lfo.out } }
      osc:   { class: sine_tone,   preset:mono, in:{ hz:sh.out } },         
      af:    { class: audio_file_out,           in:{ in:osc.out } args:{  fname:"$/out.wav"} }
    }
  }
}
```

![Example 2](svg/02_mod_sine.svg "`ex_02_mod_sine` processing network")

The `osc` instance in this example uses a `preset` statement. This will have
the effect of applying the class preset `mono` to the `osc` when it is 
instantiated. Based on values in the `mono` preset the `osc` will therefore initially have 
a single audio channel with an amplitude of 0.75.

In this example the sample and hold unit  is necessary to convert the audio signal, which
is internally represented as a vector value, to a scalar
value which is suitable as a `coeff` type value for the `hz` variable of the audio oscillator.

Here is the `sample_hold` class description:

```
sample_hold: {
  vars: {
    in:        { type:audio,   flags:["src"],  doc:"Audio input source." },
    period_ms: { type:ftime,   value:50.0,     doc:"Sample period in milliseconds." },
    out:       { type:sample,  value:0.0,      doc:"First value in the sample period." },
    mean:      { type:sample,  value:0.0,      doc:"Mean value of samples in period." },
  }
}
```

The `sample_hold` class works by maintaining a buffer of the previous `period_ms` millisecond
samples. It then outputs two values based on this buffer. `out` is simply the first
value from the buffer, and 'mean' is the average of all the values in the buffer.

### Example 03: Presets

One of the fundamental features of __caw__ is the ability to build
presets which can set the network, or a given processor, to a particular state.

`ex_02_mod_sine` showed the use of a class preset to set the number of
audio channels generated by the audio oscillator.  `ex_03_presets` shows
how presets can be specified and applied for the entire network.

In this example four network presets are given in the  `presets` statement 
and the "a" preset is automatically applied once the network is created
but before it starts to execute.

If this example was run in real-time it would also be possible to apply
the the presets while the network was running.

``` JSON
ex_03_presets: {

  dur_limit_secs:5.0,
  preset: "a",

  network: {

    procs: {
      lfo:   { class: sine_tone, args:{ hz:3, dc:440, gain:[110 120] }},
      sh:    { class: sample_hold,    in:{ in:lfo.out } },
      osc:   { class: sine_tone,      in:{ hz:sh.out } }, 
      af:    { class: audio_file_out, in: { in:osc.out } args:{  fname:"$/out.wav"} }
    }

    presets: {
          a: { lfo: { hz:1.0,       dc:[880 770] }, osc: { gain:[0.95,0.8] } },
          b: { lfo: { hz:[2.0 2.5], dc:220 }, osc: { gain:0.75 } },
          c: { lfo: a880 },
          d: [ a,b,0.5 ]          
    }
  }
}

```

This example also shows how to apply `args` or `preset` values per channel.

Audio signals in __caw__ can contain an arbitrary number of signals.
As shown by the `sine_tone` class the count of output channels (`sine_tone.ch_cnt`) 
is up to the network designer.  Processors that receive  and process incoming 
audio will often expand the count of internal audio processors to match
the count of channels they must handle.  The processor variables are
then automatically duplicated for each channel so that each channel can be controlled
independently.

One of the simplest ways to address the individual channels of a
processor is by providing a list of values in a preset specification.
Several examples of this are shown in the presets contained in then network
`presets` dictionary in `ex_03_presets`.  For example the preset
`a.lfo.dc` specifies that the DC offset of first channel of the LFO
should be 880 and the second channel should be 770.

Any processor variable that has multiple channels may be set with a
list of values. If only a single value is given (e.g. `b.lfo.dc`) then
the same value is applied to all channels.

Note that if a processor specifies a class preset with a `preset`
statement, as in the `osc` processor in `ex_02_mod_sine`, or sets
initial values with an `args` statement, these
values will be applied to the processor when it is instantiated, but
may be overwritten when the network preset is applied.  For example,
`osc` will be created with the values specified in `args`, however
when network preset "a" is applied `lfo.hz` will be overwritten with 1.0 and the
two channels of `lfo.dc` will be overwritten with 880 and 770 respectively.

When a preset is specified as a list of three values then it is interpretted
as a 'dual' preset.  The applied value of 'dual' presets are found by
interpolating between the matching values of the presets named in the
first two elements of the list using the third element as the interpolation
coefficient.  

Preset "d" specifies an interpolation between two presets "a" and "b"
where the point of interpolation is set by the third parameter, in this case 0.5.
In the example the values applied will be:

variable | channel |  value | equation
---------|---------|--------|-------------------------------------------------
lfo.hz   |    0    |   1.50 | a.lfo.hz[0] + (b.lfo.hz[0] - a.lfo.hz[0]) * 0.5 
lfo.hz   |    1    |   1.75 | a.lfo.hz[0] + (b.lfo.hz[1] - a.lfo.hz[0]) * 0.5 
lfo.dc   |    0    | 550.00 | a.lfo.dc[0] + (b.lfo.dc[0] - a.lfo.dc[0]) * 0.5 
lfo.dc   |    1    | 495.00 | a.lfo.dc[1] + (b.lfo.dc[0] - a.lfo.dc[1]) * 0.5 

Notice that the interpolation algorithm attempts to find matching channels between
the variables named in the presets, however if one of the channels does not exist 
on either preset then it uses the value from channel 0.

TODO: Check that this accurately describes preset interpolation.

### Example 04 : Programming

```
ex_04_program: {
    
  dur_limit_secs: 10.0,
        
  network {
    procs: {
          tmr: { class: timer,                               args:{ period_ms:1000.0 }},
          cnt: { class: counter,  in: { trigger:tmr.out },   args:{ min:0, max:3, inc:1, init:0, mode:modulo } },
          log: { class: print,    in: { in:cnt.out, eol_fl:cnt.out }, args:{ text:["my","count"] }}
        }
  }
}
```

![Example 4](svg/04_program.svg "`ex_04_program` processing network")


This program demonstrates how __caw__ passes messages between processors.
In this case a timer generates a pulse every 1000 milliseconds
which in turn increments a modulo 3 counter. The output of the counter
is then printed to the console by the `print` processor.

This program should output:

```
: my : 0.000000 : count
info: : Entering runtime.
: my : 1.000000 : count
: my : 2.000000 : count
: my : 0.000000 : count
: my : 1.000000 : count
: my : 2.000000 : count
: my : 0.000000 : count
: my : 1.000000 : count
: my : 2.000000 : count
: my : 0.000000 : count
: my : 1.000000 : count
```

Notice that the __print__ processor has an _eol_fl_ variable. When
this variable receives any input it prints the last value in the
_text_ list and then a newline.  In this example, although `log.in`
and `log.eol_fl` both receive values from `cnt.out`, since the
`eol_fl` connection is listed second in the `in:{...}` statement it
will receive data after the `log.in`. The newline will therefore
always print after the value received by `log.in`.


### Example 05:  Processors with expandable numbers of inputs

`ex_05_mult_inputs` extends `ex_04_program` by including a __number__ and __add__ processor.
The __number__ processor acts like a register than can hold a single value.
As used here the __number__ processor simply holds the constant value '3'.
The __add__ processor then sums the output of _cnt_ and _numb_.

```
ex_05_mult_inputs: {
    
  dur_limit_secs: 10.0,
        
  network {
    procs: {
      tmr:   { class: timer,                               args:{ period_ms:1000.0 }},
      cnt:   { class: counter,  in: { trigger:tmr.out },   args:{ min:0, max:3, inc:1, init:0, mode:modulo } },
      numb:  { class: number,                              args:{ value:3 }},
      sum:   { class: add,      in: { in0:cnt.out, in1:numb.value } },
      print: { class: print,    in: { in0:cnt.out, in1:sum.out, eol_fl:sum.out }, args:{ text:["cnt","add","count"] }}
     }
  }
}
```

![Example 5](svg/05_mult_inputs.svg "`ex_05_mult_inputs` processing network")

The notable new concept introduced by this program is the concept of
__mult__ variables.  These are variables which can be instantiated
multiple times by referencing them in the `in:{...}` statement and
including an integer suffix.  The _in_ variable of both __add__ and
__print__ have the __mult__ attribute specified in their class descriptions.
In this program both of these processors have two `in` variables:
`in0` and `in1`.  In practice they may have as many inputs as the
network designer requires.

The ability to define processors with a programmable count of inputs or output
of a given type is a key feature to any data flow programming scheme.
For example consider an audio mixer.  The count of signals that it may
need to combine can only be determined from the context in which it is used.
Likewise, as in this example, a summing processor should be able to
form a sum of any number of inputs.

### Example 06: Connecting __mult__ inputs

This example shows how the `in:{...}` statement notation can be used
to easily create and connect many `mult` variables in a single 
connection expression.

```
ex_06_mult_conn: {

  dur_limit_secs: 5.0,

  network: {
    procs: {
      osc:    { class: sine_tone, args: { ch_cnt:6, hz:[110,220,440,880,1760,3520] }},
      split:  { class: audio_split, in:{ in:osc.out }, args: { select:[ 0,0, 1,1, 2,2 ] } },
      
      // Create merge.in0,in1,in2 by iterating across all outputs of 'split'.
      merge_a: { class: audio_merge, in:{ in_:split.out_ } },
      af_a:    { class: audio_file_out, in:{ in:merge_a.out },  args:{ fname:"$/out_a.wav" }}
      
      // Create merge.in0,in1 and connect them to split.out0 and split.out1
      merge_b:  { class: audio_merge, in:{ in_:split.out0_2 } },
      af_b:     { class: audio_file_out, in:{ in:merge_b.out },  args:{ fname:"$/out_b.wav" }}
      
      // Create merge.in0,in1 and connect them both to split.out1
      merge_c:  { class: audio_merge, in:{ in0_2:split.out1 } },
      af_c:     { class: audio_file_out, in:{ in:merge_c.out },  args:{ fname:"$/out_c.wav" }}
      
      
    }
  } 
}
```

![Example 6](svg/06_mult_conn.svg "`ex_-6_mult_conn` processing network")

The audio source for this network is a six channel signal generator,
where the frequency is each channel is incremented by an octave.
The _split_ processor then splits the audio signal into three
signals where the channels are distributed to the output signals
based on the map given in the `select` list. The _split_ processor
therefore has a a single input variable `in` and three output
variables `out0`,`out1` and `out2`.

The __audio_split__ class takes a single signal and splits it into multiple signals.
The __audio_merge__ class takes multple signals and concatenates them into a single signal.
Each of the three merge processor (merge_a,merge_b,merge_c) in `ex_06_mult_conn` 
demonstrates three different ways of selecting multiple signals to merge
in with a single `in:{...}` statement expression.

The goal of this example is to show that fairly  complex connections
between a source and destination processor can be achieved with
a single `in:{...}` statement.  This syntax results in concise
network descriptions that are easier to read and modify then making lists
of individual connections between source and destination variables.

#### Connect to all available source variables on a single source processor.

```
merge_a: { class: audio_merge, in:{ in_:split.out_ } },
```
`merge_a` creates three input variables (`in0`,`in1` and `in2`) and connects them
to three source variables (`split.out0`,`split.out1`, and `split.out2`).
The equivalent but more verbose way of stating the same construct is:
`merge_a: { class: audio_merge, in:{ in0:split.out0, in1:split.out1, in2:split.out2 } }`

Aside from being more compact, the only other advantage to using the `_` (underscore)
suffix notation is that the connections will expand and contract with 
the count of outputs on _split_ should they change without having to change
the code.

#### Connect to a select set of source variables on a single source processor.

```
merge_b:  { class: audio_merge, in:{ in_:split.out0_2 } },
```

`merge_b` uses the `in:{...}` statement _begin_,_count_ notation
to select the source variables for the connection.  This statement
is equivalent to: `merge_b:  { class: audio_merge, in:{ in0:split.out0, in1:split.out1 } },`.
This notations takes the integer preceding the suffix underscore
to select the first source variable (i.e. `split.out0`) and
the integer following the underscore as the count of successive
variables - in this case 2. To select `split.out1` and `split.out2`
the `in:{...}` statemennt could be changed to `in:{ in_:split.out1_2 }`.
Likewise `in:{ in_:split.out_ }` can be seen as equivalent to: 
`in:{ in_:split.out0_3 }` in this example.

#### Connect multiple destination variable to a single source processor variable.

The _begin_,_count_ notation can also be used on the destination
side of the `in:{...}` statment expression. 

```
merge_c:  { class: audio_merge, in:{ in0_2:split.out1 } },
```
`merge_c` shows how to create two variables `merge_c.in0` and `merge_c.in1`
and connect both to `split.out1`.  Note that creating and connecting
using the _begin_,_count_ notation is general. `in:{ in1_3:split.out0_2 }`
produces a different result than the example, but is equally valid.

TODO: 
- Add the 'no_create' attribute to the audio_split.out.

- What happens if the same input variable is referenced twice in an `in:{}` statement?
An error should be generated.


### Example 07: Processor suffix notiation


As demonstrated in `ex_-6_mult_conn` variables are identified by their label
and an integer suffix id.  By default, for non __mult__ variables, the suffix id is set to 0.
Using the `in:{...}` statement however variables that have the 'mult' attribute
can be instantiated multiple times with each instance having a different suffix id.

Processors instances use a similar naming scheme; they have both a text label
and a suffix id.

```
ex_07_proc_suffix: {
  dur_limit_secs: 5.0,

  network: {
    procs: {
      osc:    { class: sine_tone, args: { ch_cnt:6, hz:[110,220,440,880,1760, 3520] }},
      split:  { class: audio_split, in:{ in:osc.out }, args: { select:[ 0,0, 1,1, 2,2 ] } },

          g0: { class:audio_gain, in:{ in:split0.out0 }, args:{ gain:0.9} },
          g1: { class:audio_gain, in:{ in:split0.out1 }, args:{ gain:0.5} },
          g2: { class:audio_gain, in:{ in:split0.out2 }, args:{ gain:0.2} },
                  
      merge: { class: audio_merge, in:{ in_:g_.out } },
      af:    { class: audio_file_out, in:{ in:merge.out },  args:{ fname:"$/out_a.wav" }}
    }
  } 
}

```

![Example 7](svg/07_proc_suffix.svg "`ex_07_proc_suffix` processing network")

In this example three __audio_gain__ processors are instantiated with
the same label 'g' and are then differentiated by their suffix id's:
0,1, and 2.  The merge processor is then able to connect to them using
a single `in:{...}` expression, `in_:g_.out` which iterates over the
gain processors suffix id. This expression is very similar to the
`merge_a` connection expression in `ex_06_mult_conn`: `in_:split.out_`
which iterated over the label suffix id's of the `split.out`.  In this
case the connection is iterating over the label suffix id's of the
networks processors rather than over a processors variables.

Note also that the _begin_,_count_ notation that allows specific
variables to be selected can also be used here to select specific
ranges of processors.

__Beware__ however that when a processor is created with a specified
suffix id it will by default attempt to connect to a source processor
with the same suffix id. This accounts for the fact that the
__audio_gain__ `in:{...}` statements must explicitely set the suffix
id of _split_ to 0.  (e.g. `in:split0.out0` ).  Without the explicit
processor label suffix id (e.g. `in:split.out0`) in `g1: {...}` and
`g2: {...}` the interpretter would attempt to connect to the
non-existent procesor `split1` and `split2` - which would trigger a
compilation error.


TODO:
- Using suffix id's this way will have cause problems if done inside a poly. Investigate.

- Should we turn off the automatic 'same-label-suffix' behaviour except when inside a `poly` network?

- How general is the 'in' statement notation? Can underscore notation be 
used simultaneously on both the processor and the variable?


### Example 08: Instantiating variables from the `args:{...}` statement.

Previous examples showed how to instantiate __mult__  variables in the `in:{...}` statement.
This example shows how to instantiate __mult__ variables from the `args:{...}` statement.

An `audio_mix` processor is the perfect motivator for this feature.
A mixer is a natural example of a processor that requires a context dependent number of inputs.
The slight complication however is that every input also has a gain coefficient
associated with it that the user may want to set.

```
ex_08_mix: {
  non_real_time_fl:true,
  dur_limit_secs:5.0,
      
  network: {

    procs: {
      osc0:  { class: sine_tone, args: { hz:110 } },
      osc1:  { class: sine_tone, args: { hz:220 } },

      // Instantiate gain:0 and gain:1 to control the input gain of in:0 and in:1.
      mix:   { class: audio_mix, in: { in_:osc_.out }, args:{ igain0:[0.8, 0], igain1:[0, 0.2] } },
      af:    { class: audio_file_out, in: { in:mix.out } args:{  fname:"$/out.wav"} }
    } 
  }          
}
```

![Example 8](svg/08_mix.svg "`ex_08_mix` processing network")

Notice that the `mix` processor  instantiates two stereo input channels in the `in:{...}` statement
and then assigns initial gain values to each individual channel. If a scalar value was given instead of a
list (e.g. `igain0:0.8`) then the scalar value would be assigned to all channels

### Example 09: Polyphonic subnet

This example introduces the __poly__ construct.  In previous examples when the
network used multiple copies of the same processor they were manually constructed - each with
a unique suffix id.  The __poly__ construct allows whole sub-networks to be duplicated
and automatically assigned unique suffix id's.

```
ex_09_simple_poly: {
   
  non_real_time_fl:true,
  dur_limit_secs: 5.0,

  network: {

    procs: {

      // LFO gain parameters - one per poly voice
      g_list:  { class: list, args: { in:0, list:[ 110f,220f,440f ]}},

      // LFO DC offset parameters - one per poly voice
      dc_list: { class: list, args: { in:0, list:[ 220f,440f,880f ]}},
          
      osc_poly: {
        class: poly,
        args: { count:3 },  // Create 3 instances of 'network'.
           
        network: {
          procs: {
            lfo:  { class: sine_tone,   in:{ _.dc:_.dc_list.value_, _.gain:_.g_list.value_ }  args: { ch_cnt:1, hz:3 }},
            sh:   { class: sample_hold, in:{ in:lfo.out }},
            osc:  { class: sine_tone,   in:{ hz: sh.out }},
         }
       }               
     }

     // Iterate over the instances of `osc_poly.osc_.out` to create one `audio_merge`
     // input for every output from the polyphonic network.
     merge: { class: audio_merge,    in:{ in_:osc_poly.osc_.out}, args:{ gain:1, out_gain:0.5 }},
     af:    { class: audio_file_out, in:{ in:merge.out }          args:{ fname:"$/out.wav"} }
    }
  }
}
```

![Example 9](svg/09_simple_poly.svg "`ex_09_simple_poly` processing network")

This program instantiates three modulated sine tones each with a different set of parameters.

Notice the _lfo_ `in:{...}` statement for the `dc` variable
connection. The statement contains three underscores.  The first
underscore indicates that the connection should be made to all of the
_lfo_ processors in the subnet (i.e. `lfo0`,`lfo1`,`lfo2`). The second
underscore indicates that the source is located outside the subnet.
The compiler will iterate through the network, in execution order,
looking for a processor named `dc_list` as the source. The last
underscore indicates that that the connections should begin with
`g_list.value0` and iterate forward to locate `glist.value1` and
`glist.value2` to locate the other source variables.

One subtle characteristic of the the _poly_ subnet is that the
internal connections (`lfo->sh->osc`) do not specifiy processor
id's.  By default the `in:{...}` assumes that source processors
and desination processors share the same id.  This allows the
suffix id to be dropped from the source processor and thereby to simplify
the syntax for connecting sub-network processors.

Also note that _poly_ to external connections are simply made
by referring to the poly source by name to locate the source processor.
This is shown in the `merge` input statement `in:{ in_:osc_poly.osc_.out}`.

The final characteristic to note about the poly construct
is the use of the 'parallel_fl' attribute.  When this flag is
set the subnets will run concurrently in separate threads.
Since no connections between subnets is possible, and no other
processors can run, while the subnets are running this is
always safe.

### Example 10: Heterogeneous polyphonic subnet

In the previous example each of the three voices shared the same network
structure.  In this example there are two voice with different
networks.

```
ex_10_hetero_poly: {
   
  non_real_time_fl:true,
  dur_limit_secs: 5.0,

  network: {
    procs: {

      g_list:  { class: list, args: { in:0, list:[ 110f,220f,440f ]}},
      dc_list: { class: list, args: { in:0, list:[ 220f,440f,880f ]}},
          
      osc_poly: {
        class: poly,
            
        args: { parallel_fl:true },  
           
        network: [
            
          // network 0
          {
            procs: {
              lfo:  { class: sine_tone,   in:{ _.dc:_.dc_list.value_, _.gain:_.g_list.value_ }  args: { ch_cnt:1, hz:3 }},
              sh:   { class: sample_hold, in:{ in:lfo.out }},
              osc_a:  { class: sine_tone,   in:{ hz: sh.out }},
            }
          },
              
          // network 1
          {
            procs: {
              osc_b:  { class: sine_tone,   args:{ hz:55 }},
            }
          }
        ]               
      }

      // Iterate over the instances of `osc_poly.osc_.out` to create one `audio_merge`
      // input for every output from the polyphonic network.
      merge: { class: audio_merge,    in:{ in0:osc_poly.osc_a.out, in1:osc_poly.osc_b.out}, args:{ gain:1, out_gain:0.5 }},
      af:    { class: audio_file_out, in:{ in:merge.out }          args:{ fname:"$/out.wav"} }
    }
  }
}
```

Since the structure of the two subnets is different the very compact
`in:{...}` statements used to connect the `merge` processor to
the output of each of the `osc` processors is no longer possible.
However, this example demonstrates how `caw`can be used to
run heterogeneous networks concurrently thereby makeing better
use of available hardware cores.




### Example 11: Feedback

```
ex_11_feedback: {
  non_real_time_fl:true,
  max_cycle_count:   10,

  network: {
    procs: {
      a:   { class: number,  log:{out:0}, args:{ in:1 }},
      b:   { class: number,  log:{out:0}, args:{ in:2 }},
              
      sum: { class: add,  in: { in0:a.out, in1:b.out }, out: {out:b.in }, log:{out:0} }
    }
  }
}
```
![Example 11](svg/11_feedback.svg "`ex_11_feedback` processing network")

This example demonstrates how to achieve a feedback connection using
the `out:{...}` statement.  Until now all the examples have been
forward connections. That is processors outputs act as sources to
processes that execute later.  The `out:{...}` statement allows
connections to processes that occur earlier in the execution chain.  The trick to making this
work is to be sure that the destination processor does not depend on
the variable receiving the feedback having a valid value on the
very first cycle of network execution - prior to the source processor
executing.  One way to achieve this is to set the value of the
variable receiving the feedback to a default value in the `args:{...}`
statement. This approach is used here with the `b.in` variable.

This example also introduces the `log:{...}` statement.
The `log:{...}` statement has the form `log:{ <var>:<suffix_id>, <var>:<suffix_id> ... }`.
Any variables included in the statement will be logged to the console
whenever the variable changes value.  This is often a more convenient
way to monitor the changing state of the network then using calls to `print`.
The output is somewhat cryptic but contains most of information to debug a program.

```
: exe cycle:    process:   id:       variable: id     vid     ch :         : : type:value  : destination
: ---------- ----------- ----- --------------- --     ---    -----             ------------: -------------
:        0 :          a:    0:            out:  0 vid:  2 ch: -1 :         : : <invalid>: 
:        0 :          a:    0:            out:  0 vid:  2 ch: -1 :         : : i:1 : 
:        0 :          b:    0:            out:  0 vid:  2 ch: -1 :         : : <invalid>: 
:        0 :          b:    0:            out:  0 vid:  2 ch: -1 :         : : i:2 : 
:        0 :        add:    0:            out:  0 vid:  0 ch: -1 :         : : d:3.000000 :  dst:b:0.in:0: 
info: : Entering runtime.   
:        0 :          a:    0:            out:  0 vid:  2 ch: -1 :         : : i:1 :  dst:add:0.in:0: 
:        0 :          b:    0:            out:  0 vid:  2 ch: -1 :         : : i:2 :  dst:add:0.in:1: 
:        0 :        add:    0:            out:  0 vid:  0 ch: -1 :         : : d:3.000000 :  dst:b:0.in:0: 
:        1 :          b:    0:            out:  0 vid:  2 ch: -1 :         : : i:3 :  dst:add:0.in:1: 
:        1 :        add:    0:            out:  0 vid:  0 ch: -1 :         : : d:4.000000 :  dst:b:0.in:0: 
:        2 :          b:    0:            out:  0 vid:  2 ch: -1 :         : : i:4 :  dst:add:0.in:1: 
:        2 :        add:    0:            out:  0 vid:  0 ch: -1 :         : : d:5.000000 :  dst:b:0.in:0: 
:        3 :          b:    0:            out:  0 vid:  2 ch: -1 :         : : i:5 :  dst:add:0.in:1: 
```

#### Log Column Descriptions

Column      | Description
------------|---------------------------------------------------------------------
exe cycle   | The `exe cycle` value give the execution cycle index.
            | Each time the network completes a cycle this index advances.
process id  | The process label and suffix id of the variable
variable id | The variable label and variable suffix id.
vid         | System assigned unique (per process) id.
ch          | Channel index or -1 if the variable is not channelized.
            | Variables may be channelized if the audio signal they are applied to have multiple channels.
type:value  | Data type and current value of the variable.

### Example 11: Audio Feedback

Coming soon.

### Example 12: User Defined Processor

__caw__ user defined processor act somewhat like functions in a procedural programming language.
They allow a network designer to create an arbitrarily complex network but then
implement a well defined interface to it. The network can then be instantiated just like
a built-in process with a limited set of inputs and outputs. This hides the complexity of
the implementation of the network from the user and makes for simpler top level networks.

Here is a user defined processor which implements an oscillator with internal amplitude and frequency modulators.
```
mod_osc: {

  vars: {
    hz:            { proxy:hz_lfo.dc,            doc:"Audio frequency" },
    hz_mod_hz:     { proxy:hz_lfo.hz,            doc:"Frequency modulator hz" },
    hz_mod_depth:  { proxy:hz_lfo.gain,          doc:"Frequency modulator depth" },
    amp_mod_hz:    { proxy:amp_lfo.hz,           doc:"Amplitude modulator hz" },
    amp_mod_depth: { proxy:amp_lfo.gain,         doc:"Amplutide modulator depth."},
    mo_out:        { proxy:ogain.out flags:[out] doc:"Oscillator output."},
  },
        
  network: {
    procs: {
      // Frequency modulating LFO
      hz_lfo:   { class: sine_tone,   args: { ch_cnt:1 }}
      hz_sh:    { class: sample_hold, in:{ in:hz_lfo.out }}

      // Amplitude modulating LFO
      amp_lfo:  { class: sine_tone,   args: { ch_cnt:1 }}
      amp_sh:   { class: sample_hold, in:{ in:amp_lfo.out }}

      // Audio oscillator
      osc:    { class: sine_tone,   in:{ hz: hz_sh.out }}        
      ogain:  { class: audio_gain,  in:{ in:osc.out, gain:amp_sh.out}}
    }
          
    presets: {
        net_a: { hz_lfo: { dc:220, gain:55 }, amp_lfo: { gain:0.8 } },
        net_b: { hz_lfo: { dc:110, gain:25 }, amp_lfo: { gain:0.7 } },
    }
  }
}

```

The structure of a user defined procedure is the same as a __caw__ top level program with the
addition of the __vars__ dictionary. The elements of the __vars__ dictionary are a simplified
version of the variable descriptions from the processor class description record.
Each user defined __var__ element creates an input or output port for the user defined process using
the `proxy` keyword.  Output ports are distinguished from input ports by the `out` attribute as
shown in the `mo_out` port. 

The `mod_osc` user defined processor is instantiated and used just like a built-in processor.
```
user_defined_proc_12 : {
  non_real_time_fl: true,
  dur_limit_secs:   5,
	  
  network: {
    procs: {
      sub_osc: { class: mod_osc  args:{ hz:220, hz_mod_hz:3, hz_mod_depth:55, amp_mod_hz:2, amp_mod_depth:0.5 }},             
      af:      { class: audio_file_out, in:{ in:sub_osc.mo_out } args:{ fname:"$/out.wav"}}           
   }
 }
}
```

### Example 13: Global Variables

See the sampler wavetable.




### Appendix:

#### Network Parameters:

Label                 | Description
----------------------|------------------------------------------------------------------------
`non_real_time_fl`    | Run the program in non-real-time for `max_cycle_count` cycles.
`frames_per_cycle`    | Count of audio sample frames per network execution cycle.
`sample_rate`         | Default system audio sample rate.
`max_cycle_count`     | Maximum count of network cycles to execute in non-real-time mode.
`dur_limit_secs`      | Set `max_cycle_count` as (`sample_rate` * `dur_limit_secs`)/`frames_per_cycle`.
`print_class_dict_fl` | 
`print_network_fl`    |


#### `log:{...}` statement data type abbreviations:

Type | Description
-----|-------------
b    | bool
u    | unsigned
i    | int
f    | float
d    | double
s    | string
t    | time
c    | cfg
abuf | audio
fbuf | spectrum
mbuf | MIDI


### Execution Model

### Some Invariants

#### Network Invariants
- A given variable instance may only be connected to a single source.
- Once a processor is instantiated the count and data types of the variables is fixed.
- Once a processor is instantiated no new connections can be created or removed. (except for feedback connections?)
- If a variable has a source connection then it cannot be assigned a value.
- Processors always execute in order from top to bottom.

#### Internal Proc Invariants
- The _value() function will be called once for every new value received by a variable.
