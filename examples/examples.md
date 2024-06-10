


### Example 01 - Write a sine signal to an audio file.


__caw__ programs are described using a slightly extended form of JSON. 
In this example the program is contained in the dictionary labeled `sine_file_01` and
the preceeding fields (e.g. `base_dir`,`proc_dict`,`subnet_dict`, etc.) contain
system parameters that the program needs to compile and run the program.

```
{
  base_dir:    "~/src/caw/examples",
  proc_dict:   "~/src/caw/examples/proc_dict.cfg",
  mode: non_real_time,
    
  programs: {

    sine_file_01: {

      durLimitSecs:5.0,

      network: {

        procs: {
          osc: { class: sine_tone },
          af:  { class: audio_file_out, in: { in:osc.out } args:{  fname:"$/out.wav"} }
        } 
      }      
    }
  }
}

```

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
The _project directory_ is automatically created when the program is run.


### Processor Class Descriptions

- _processors_ are collections of named __variables__ which are defined in the
processor class file named by the `proc_dict` system parameter field.

Here are the class specifications for `sine_tone` and `audio_file_out`.

```
sine_tone: {
  vars: {
      srate:     { type:srate, value:0,         doc:"Sine tone sample rate. 0=Use default system sample rate"}
      chCnt:     { type:uint,  value:2,         doc:"Output signal channel count."},
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
      mono:  { chCnt:1, gain:0.75 }
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

Based on the `sine_tone` class all the default values for the signal generator
are apparent.  With this information it is clear that the audio file
written by `sine_file_01` contains a stereo (`chCnt`=2), 440 Hertz signal
with an amplitude of 0.8.

Note that unless stated otherwise all variables can be either input or output ports for their
proc. The `no_src` attribute on `sine_tone.out` indicates that it is an output-only
variable. The `src` attribute on `audio_file_out.in` indicates that it must be connected to
a source variable or the processor cannot be instantiated - and therefore the network it is contained
by cannot be instantiated.  Note that this isn't to say that it can't be an output variable - only
that it must be connected.

TODO: 
1. more about types - especially the non-obvious 'srate','coeff'.
Link to proc class desc reference.
2. more about presets.
3. variables may be a source for multiple inputs but only be connected to a single source.
4. change `sine_tone.chCnt` to `ch_cnt`.

### Example 02: Modulated Sine Signal

This example is an extended version of `sine_file_01` where a low frequency oscillator (LFO)
is formed using a second `sine_tone` processor and a sample and hold unit. The output
of the sample and hold unit is then used to modulate the frequency of an audio
frequency `sine_tone` oscillator.

Note that the LFO output is a 3 Hertz sine signal
with a gain of 110 (220 peak-to-peak amplitude) and an offset
of 440.  The LFO output signal is therefore sweeping an amplitude
between 330 and 550 which will be treated as frequency values by `osc`.

```
mod_sine_02: {

  durLimitSecs:5.0,

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

The `osc` instance in this example uses a `preset` statement. This will have
the effect of applying the class preset `mono` to the `osc` when it is 
instantiated. Based on the `sine_tone` class description the `osc` will therefore
have a single audio channel with an amplitude of 0.75.

In this example the sample and hold unit  is necessary to convert the audio signal to a scalar
value which is suitable as a `coeff` type value for the `hz` variable of the audio oscillator.

Here is the `sample_hold` class description:

```
sample_hold: {
  vars: {
    in:        { type:audio,   flags:["src"],  doc:"Audio input source." },
    period_ms: { type:ftime,   value:50.0,       doc:"Sample period in milliseconds." },
    out:       { type:sample,  value:0.0,      doc:"First value in the sample period." },
    mean:      { type:sample,  value:0.0,      doc:"Mean value of samples in period." },
  }
}
```

The `sample_hold` class works by maintaining a buffer of the previous `period_ms` millisecond
samples it has received. The output is both the value of the first sample in the buffer (`sh.out`)
or the mean of all the values in the buffer (`sh.mean`).


### Example 03: Presets

One of the fundamental features of __caw__ is the ability to build
presets which can set the network, or a given processor, to a particular state.

`mod_sine_02` showed the use of a class preset to set the number of
audio channels generated by the audio oscillator.  `presets_03` shows
how presets can be specified and applied for the entire network.

In this example four network presets are specified in the  `presets` statement 
and the "a" preset is automatically applied once the network is created
but before it starts to execute.

```
presets_03: {

  durLimitSecs:5.0,
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
As shown by the `sine_tone` class the count of output channels (`sine_tone.chCnt`) 
is up to the network designer.  Processors that receive  and process incoming 
audio will often expand the count of internal audio processors to match
the count of channels they must handle.  The processor variables are
then automatically duplicated for each channel so that each channel can be controlled
independently.

One of the simplest ways to address the individual channels of a
processor is by providing a list of values in a preset specification.
Several examples of this are shown in the presets contained in then network
`presets` dictionary in `presets_03`.  For example the preset
`a.lfo.dc` specifies that the DC offset of first channel of the LFO
should be 880 and the second channel should be 770.

Any processor variable that has multiple channels may be set with a
list of values. If only a single value is given (e.g. `b.lfo.dc`) then
the same value is applied to all channels.

Note that if a processor specifies a class preset with a `preset`
statement, as in the `osc` processor in `mod_sine_02`, or sets
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

### Example 04 : Event Programming

```
program_04: {
    
  durLimitSecs: 10.0,
	
  network {
    procs: {
	  tmr: { class: timer,                               args:{ period_ms:1000.0 }},
	  cnt: { class: counter,  in: { trigger:tmr.out },   args:{ min:0, max:3, inc:1, init:0, mode:modulo } },
	  log: { class: print,    in: { in:cnt.out, eol_fl:cnt.out }, args:{ text:["my","count"] }}
	}
  }
}
```

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

`mult_inputs_05` extends `program_04` by including a __number__ and __add__ processor.
The __number__ processor acts like a register than can hold a single value.
As used here the __number__ processor simply holds the constant value '3'.
The __add__ processor then sums the output of _cnt_ and _numb_.

```
mult_inputs_05: {
    
  durLimitSecs: 10.0,
	
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

The notable new concept introduced by this program is the concept of
__mult__ variables.  These are variables which can be instantiated
multiple times by referencing them in the `in:{...}` statement and
including an integer suffix.  The _in_ variable of both __add__ and
__print__ have this attribute specified in their class descriptions.
In this program both of these processors have two `in` variables:
`in0` and `in1`.  In practice they may have as many inputs as the
network designer requires.


### Example 06: Connecting __mult__ inputs

This example shows how the `in:{...}` statement notation can be used
to easily create and connect many `mult` variables in a single 
connection expression.

```
mult_conn_06: {

  durLimitSecs: 5.0,

  network: {
    procs: {
      osc:    { class: sine_tone, args: { chCnt:6, hz:[110,220,440,880,1760,3520] }},
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

The audio source for this network is a six channel signal generator,
where the frequency is each channel is incremented by an octave.
The _split_ processor then splits the audio signal into three
signals where the channels are distributed to the output signals
based on the map given in the `select` list. The _split_ processor
therefore has a a single input variable `in` and three output
variables `out0`,`out1` and `out2`.

The __audio_split__ class takes a single signal and splits it into multiple signals.
The __audio_merge__ class takes multple signals and concatenates them into a single signal.
Each of the three merge processor (merge_a,merge_b,merge_c) in `mult_conn_06` 
demonstrates a slightly different ways of selecting multiple signals to merge
in with a single `in:{...}` statement expression. 

1. Connect to all available source variables.

```
merge_a: { class: audio_merge, in:{ in_:split.out_ } },
```
`merge_a` creates three input variables (`in0`,`in1` and `in2`) and connects them
to three source variables (`split.out0`,`split.out1`, and `split.out2`).
The completely equivalent, and equally correct way of stating the same construct is:
`merge_a: { class: audio_merge, in:{ in0:split.out0, in1:split.out1, in2:split.out2 } }`

Aside from being more compact, the only other advantage to using the `_` (underscore)
suffix notation is that the connections will expand and contract with 
the count of outputs on _split_ should they change without having to change
the code.

2. Connect to a select set of source variables.

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

3. Create and connect to a selected variables.

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


As demonstrated in `mult_conn_06` variables are identified by their label
and an integer suffix id.  By default, for singular variable the suffix id is set to 0.
Using the `in:{...}` statement however variables that have the 'mult' attribute
can be instantiated multiple times with each instance having a different suffix id.

Processors instances use a similar naming scheme; they have both a text label
and a suffix id.
```
proc_suffix_07: {
  durLimitSecs: 5.0,

  network: {
    procs: {
      osc:    { class: sine_tone, args: { chCnt:6, hz:[110,220,440,880,1760, 3520] }},
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

In this example three __audio_gain__ processors are instantiated with
the same label 'g' and are then differentiated by their suffix id's:
0,1, and 2.  The merge processor is then able to connect to them using
a single `in:{...}` expression, `in_:g_.out` which iterates over the
gain processors suffix id. This expression is very similar to the
`merge_a` connection expression in `mult_conn_06`: `in_:split.out_`
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



### Some Invariants

#### Network Invariants
- A given variable instance may only be connected to a single source.
- Once a processor is instantiated the count and data types of the variables is fixed.
- Once a processor is instantiated no new connections can be created or removed. (except for feedback connections?)
- If a variable has a source connection then it cannot be assigned a value.
- Processors always execute in order from top to bottom.

#### Internal Proc Invariants
- The _value() function will be called once for every new value received by a variable.
