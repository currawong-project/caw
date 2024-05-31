


### Example 01 - Write a sine signal to an audio file.



```
{
  base_dir:    "~/src/caw/examples",
  proc_dict:   "~/src/caw/examples/proc_dict.cfg",
  mode: non_real_time,
    
  programs: {

    example_01: {

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

__caw__ programs are described using a slightly extended form of JSON. 
In this example the program is contained in the dictionary labeled `example_01` and
the preceeding fields (e.g. `base_dir`,`proc_dict`,`subnet_dict`, etc.) contain
system parameters that the program needs to compile and run the program.

When run this program will write a five second sine signal to an audio file
named `~/src/caw/examples/example_01/out.wav`.  The output file name
is formed by joining the value of the system parameter `base_dir` with 
the name of the program `example_01`.

Run the program like this:
```
caw example.cfg example_01
```

__caw__ specify and run a network of virtual processors.  The network is
described in the `procs` dictionary. 

The line beginning with `osc: {` defines an instance of a `sine_tone` processor
named `osc`. The line beginning with `af: {` defines an instance of a `audio_file_out` processor
named `af`.

In the language of __caw__ `osc` and `af`  are refered to as `processor instances` or
__procs__. 

`osc` and `af` are connected together using the `in:{}` statement in the `af` 
instance description.  The `in` statement connects `osc.out` to `af.in` and
thereby directs the output of the signal generator into the audio file.

The `args:{}` statment lists instance specific arguments used to create the 
`af` instance. In this case `af.fname` names the output file. The use of the
`$` prefix on the file name indicates that the file should be written to 
the _project directory_ which is formed by joining `base_dir` with the program name.
The _project directory_ is automatically created when the program is run.


### Processor Class Descriptions

- __procs__ are collections of named __variables__ which are defined in the
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
written by `example_01` contains a stereo (`chCnt`=2), 440 Hertz signal
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

### Example 02: Modulated Sine Signal

This example is an extended version of `example_01` where a low frequency oscillator
is formed using a second `sine_tone` processor and a sample and hold unit. The output
of the sample and hold unit is then used to module the frequency of an audio
frequency `sine_tone` oscillator.

Note that the LFO output by specifies a 3 Hertz sine signal
with a gain of 110 (220 peak to peak amplitude) and an offset
of 110.  The signal is therefore sweeping an amplitude
between 330 and 550 which will be treated as frequency values by `osc`.

```
example_02: {

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
    period_ms: { type:ftime,   value:50,       doc:"Sample period in milliseconds." },
    out:       { type:sample,  value:0.0,      doc:"First value in the sample period." },
    mean:      { type:sample,  value:0.0,      doc:"Mean value of samples in period." },
  }
}
```

The `sample_hold` class works by maintaining a buffer of the previous `ftime` millisecond
samples it has received. The output is both the value of the first sample in the buffer (`sh.out`)
or the mean of all the values in the buffer (`sh.mean`).



### Example 03: Presets

One of the fundamental features of __caw__ is the ability to build
presets which can set the network or a given processor to a particular state.

`example_02` showed the use of a class preset on the audio oscillator.
`example_03` shows how presets can be specified and applied for the entire network.

In this example four network presets are specified in the  `presets` statement 
and the "a" preset is automatically applied once the network is created
but before it starts to execute.

```
example_03: {

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
the count of channels they must handle.  The processor variables must
then be duplicated for each channel if each channel is to be controlled
independently.

One of the simplest ways to address the individual channels of a
processor is by providing a list of value in a preset specification.
Several examples of this are shown in the presets contained in network
`presets` dictionary in `example_03`.  For example the preset
`a.lfo.dc` specifies that the DC offset of first channel of the LFO
should be 880 and the second channel should be 770.

Any processor variable that has multiple channels may be set with a
list of values. If only a single value is given (e.g. `b.lfo.dc`) then
the same value is applied to all channels.

Note that if a processor specifies a class preset with a `preset`
statement, as in the `osc` processor in `example_02`, or sets
initial values with an `args` statement, these
values will be applied to the processor when it is instantiated, but
may be overwritten when the network preset is applied.  For example,
`osc` will be created with the values specified in `args`, however
when network preset "a" is applied `lfo.hz` will be overwritten with 1.0 and the
two channels of `lfo.dc` will be overwritten with 880 and 770 respectively.

Preset "d" specifies an interpolation between two presets "a" and "b"
where the point of interpolation is set by the third parameter, in this case 0.5.
In the example the values applied will be:

variable | channel |  value | equation
---------|---------|--------|-------------------------------------------------
lfo.hz   |    0    |   1.50 | a.lfo.hz[0] + (b.lfo.hz[0] - a.lfo.hz[0]) * 0.5 
lfo.hz   |    1    |   1.75 | a.lfo.hz[0] + (b.lfo.hz[1] - a.lfo.hz[0]) * 0.5 
lfo.dc   |    0    | 550.00 | a.lfo.dc[0] + (b.lfo.dc[0] - a.lfo.dc[0]) * 0.5 
lfo.dc   |    1    | 495.00 | a.lfo.dc[1] + (b.lfo.dc[0] - a.lfo.dc[1]) * 0.5 

Notice that the interpolation algorithm attempts to match channels between the presets,
however if one of the channels does not exist then it uses channel 0.

TODO: Check that this accurately describes preset interpolation.
