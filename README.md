
# Reel - a Query Language for TrailDB

![Reel logo](/doc/reel_logo.png)

Reel is a small language for querying time-series
of discrete events encoded as [TrailDBs](http://traildb.io).
The standard TrailDB API provides functionality for [filtering individual
events](http://traildb.io/docs/api/#filter-events) based on a Boolean
query but expressing more complex queries over multiple events is left to
the application, or to a language like Reel.

Reel makes it easy to express complex queries. Reel provides zero-cost
abstraction over hand-written C code using the TrailDB API: Reel
programs are compiled to optimization-friendly C code which can be
easily accessed by any language.

### What is Reel?

Consider Reel as [AWK](http://wikipedia.org/wiki/AWK)
for TrailDB, following the tradition of [data-driven
programming](http://wikipedia.org/wiki/Data-driven_programming). Whereas
AWK programs are evaluated over lines of text, Reel programs are
evaluted over a trail in TrailDB, that is, a sequence of events ordered
by time.

Similar to AWK, Reel programs are series of pattern-action
pairs which are evaluated for each event in the trail, in order.
Typically actions involve updating internal variables of the program,
such as counters, which are finally returned to the calling program or
output to a CSV file.

Figuratively, this is like pulling a strip of frames (events) from a
film reel (trail) and deciding what to do with each frame based on its
contents (pattern-action). You can also control the movement of the film
reel in the program (rewind, stop, or jump to a frame).

### Use Cases

Reel is meant for computing metrics over series of events that happen over
time. These types of queries are typically very tedious to express as SQL,
and expensive to query at scale, which is the raison d'etre for TrailDB.
Examples of such queries include:

- **Multi-event metrics** - Compute metrics over multiple events, such as
  bounce rate, i.e. a person visiting only one page at a web site before
  leaving.

- **Funnel analysis** - Analyze how many users follow a certain sequence
  of events within a given time window, e.g. from site visit to purchase.

- **Threading** - A trail may include a mixture interleaved events
  produced by many concurrent sources. For instance, consider a person
  surfing the web across many web sites. You can use Reel to group events
  related to each web site and produce metrics for each of them separately.

- **Joins** - Enrich events by joining them with external data based on a
  field or a more complex logic, and produce metrics over the enriched trails.

### Why Reel?

All the above use cases could be covered by a general-purpose
language, such as Python or C, using [the standard TrailDB
API](http://traildb.io/docs/api/). This approach may be preferable for
the trickiest use cases but in most cases it would be convenient to
use a simpler and more expressive way to write queries.

Based on our experiences, here are some of the main motivations for
using Reel:

- **Rapid** - For its intended use, Reel is much more expressive
  than a low-level language like C and thus more rapid to develop. Reel
  compiles to C so performance is at least at the level of hand-written C,
  which makes its much faster than higher-level languages such as Go or
  Python.

- **Embeddable** - By design, Reel queries are easy to embed in
  applications written in any language, either by linking them
  directly or by using a Foreign Function Interface, such as [Python's
  ctypes](https://docs.python.org/3/library/ctypes.html).

- **Extensible** - You can easily extend Reel itself by introducing new
  functions. You can build a library of domain-specific functions of your
  own on top of Reel.

- **Lightweight** - Reel is a very simple language, so its cognitive
  overhead is minimal.

## Reel Tutorial

This tutorial introduces you to all features of Reel.

### Getting Reel

1. Make sure your system has Python and `gcc` installed.

2. [Install TrailDB](http://traildb.io/docs/getting_started/).

3. Check out the Reel repo `git clone https://github.com/traildb/reel`.

There are two main modes for using Reel: By default, the `reel` command
takes a Reel program and a TrailDB and executes the program over the
TrailDB and outputs the results as CSV. Alternatively, you can use
`reel` to only compile your program for linking with an external
application (TODO: How?).

You can check that your setup works by executing

`./reel doc/00-helloworld.rl doc/hedgehog1.tdb`

If everything works smoothly, you should see output like
```
Hello,World
1,2
```

### Example Data

You can find all examples below in the `/doc/` directory in the Reel repo.
Most examples use a TrailDB, included in the repo at `/doc/hedgehog1.tdb`,
which contains one trail that looks as follows:

![Hedgehog Example 1](/doc/hedgehog1.png)

Besides the standard `time` (shown in the upper left corner of each
frame) and `uuid` fields, there are two additional fields: `color` and
`hedgehog`. As you can see in the illustration above, the frames are
either yellow, blue, or the color is undefined (white). Some frames have
a hedgehog. Your TrailDBs are surely more interesting but they work the
same way with Reel.

### Syntax

#### Example: [00-helloworld.rl](/doc/00-helloworld.rl)
Print out hello world.
```Go
# variable declarations
var Hello uint
var World uint

# a pattern matching the beginning of the trail
begin:
    # actions
    set Hello 1
    set World 2

```
- **Variable declarations** are listed at the top of the program. All
  variables used in the program must be declared here.

- **Patterns**, like `begin` above, are listed at the top level and they
  are evaluated in order for every event. An event can match zero or more
  patterns, not just one.

- **Actions** are evaluated for each matching pattern. Reel is a
  whitespace sensitive language, like Python. Actions need to be indended
  under the pattern.

- **Comments** are prefixed by `#`.

### Scalar Types

Reel is a strongly typed language with static (compile-time) type checking.
The types are defined in the variable declarations, like `var Hello uint` above.

Reel has the following scalar types:

 - *uint* - a 64-bit unsigned integer
 - *item* - [a TrailDB item](http://traildb.io/docs/technical_overview/#data-model),
   i.e a field-value pair.

Much of the power of Reel comes from its first-class support of TrailDB items.
Literal references to TrailDB fields and items are prefixed with a dollar sign, `$`.
Variables don't have a prefix.

By convention, variables starting with an uppercase letter are shown in
the output. Variables starting with an underscore are reserved.

#### Example: [01-counter.rl](/doc/01-counter.rl)
Count the number of hedgehogs.
```Go
var Hedgehogs uint

if $hedgehog:
    inc Hedgehogs 1
```
Execute the example by running `./reel doc/01-counter.rl doc/hedgehog1.tdb`.
As seen in the picture above, the result should be 6 hedgehogs.

This example uses one variable, `Hedgehogs`, which is incremented every
time we find a hedgehog. The pattern `if $hedgehog` matches every event
that has a non-empty value in the field `$hedgehog`. The action `inc`
increments its first argument, which needs to be of type `uint`, by the
amount specified in the second `uint` argument.

#### Example: [02-colors.rl](/doc/02-colors.rl)
Count the number of colors.
```Go
var Blues uint
var Yellows uint

if $color $color='yellow':
    inc Yellows 1

if $color $color='blue':
    inc Blues 1
```
This program should output 3 blues and 5 yellows.

In contrast to the previous example, which matched any event with
a non-empty value for `$hedgehog`, this example matches a specific
value of the field. Reel provides special syntax for resolving
specific values or **item literals**, such as `$color='yellow'` above.
Item literals are resolved when the program is initialized using
[tdb_get_item](http://traildb.io/docs/api/#tdb_get_item), which allows
Reel to evaluate the pattern quickly.

You need to specify both the field name, `$color`, as well as the value,
`yellow`, since the same value may exist in multiple fields. The pattern
matches when the field `$color` in the event has the value `yellow`.

### Lookup Tables

Besides scalar types, `uint` and `item`, Reel supports special arrays,
called (lookup) tables, which map items to other values. Lookup tables
come in handy when dealing with fields with a high cardinality. Handling
cases like this with item literals, like in the previous example, would
not be very convenient.

#### Example: [03-table.rl](/doc/03-table.rl)
Count the number of colors using a lookup table.

```Go
var Colors table:$color->uint

if 1:
    inc Colors[$color] 1
```
Like the previous exaple, this example counts the number of colors but
this time using a lookup table. The variable declaration `var Colors
table:$color->uint` defines a table `Colors` that maps all values of
`$color` to `uint`. The table is created at the time of initialization,
so no run-time memory allocations are needed.

The pattern `if 1` evaluates to true for each event. Because of this, you
can see that this example counts the number of whites as well. Can you
modify the program so that it counts only blues and yellows?

#### Example: [04-preset-table.rl](/doc/04-preset-table.rl)
Score colors based on a preset table.

```Go
var Colors table:$color->uint
var Scores table:$color->uint

if 1:
    inc Colors[$color] Scores[$color]
```
A very powerful feature of Reel is that you can assign values to
variables in the calling application. Hence you can use variables
to parameterize behavior of the program.

Try executing the above program as usual:

`./reel doc/04-preset-table.rl doc/hedgehog1.tdb`

The result will be all zeroes: The program increments `Colors` based
on the value defined in `Scores`, which is zeroes by default.

Now try this:

`./reel doc/04-preset-table.rl --set Scores=@doc/scores.csv doc/hedgehog1.tdb`

The `--set` option can be used to preset values to variables. In this case,
we are populating the `Scores` tables based on the values in
[doc/scores.csv](/doc/scores.csv).

### Handling Time

TrailDB is all about events over time, so Reel handles time natively.
The timestamp of the current event is accessible through the field
`$time` that has the type `uint`.

#### Example: [05-time.rl](/doc/05-time.rl)
Find pairs of hedgehogs that are not more than `window` time units apart.
```Go
var Pairs uint
var hedgehog_time uint
var window uint

if $hedgehog:
    if hedgehog_time and time_before hedgehog_time window:
        inc Pairs 1
    set hedgehog_time $time
else:
    set hedgehog_time 0
```
This example finds all hedgehogs that are immediately followed by
another hedgehog that comes at most `window` seconds after the first
one. This is accomplished by storing the time of each hedgehog seen in
`hedgehog_time` and by comparing it to the current time with the
`time_before` pattern.

Note that `window` is zero by default so no pairs are found. Try

`./reel doc/04-preset-table.rl --set window=1 doc/hedgehog1.tdb`

that captures the pair at 5-6 seconds. By setting the `window=4`
you can also capture the pair at 1-5 seconds etc.

### Control Flow

By default, Reel evaluates every event in the trail. You
can control what events Reel sees by using TrailDB's [event
filter](http://traildb.io/docs/api/#filter-events). You can
also control the flow of events using **control flow** statements
of Reel:

- `begin` is evaluated before the first event, even when there may be no
  events due to event filtering. It must be the first pattern of the
  program.
- `end` is evaluated after the last event, before the program exits. It
  must be the last pattern of the program.
- `stop` early exits evaluation of the trail.
  Note that the `end` pattern is evaluated immediately after calling
  `stop`.
- `next` jumps to the next event immediately, without evaluating
  rest of the pattens.
- `rewind` jumps to the first event, without evaluating `begin` again.
- `setpos pos` jumps to the arbitrary position in the trail, stored in
  the `uint` variable `pos`. You can retrieve the current position in the
  trail from a special variable `_POS`.

#### Example: [06-rewind.rl](/doc/06-rewind.rl)
Count the number of blues in the trail if the last event of the trail is yellow.
```Go
var last_is_yellow uint
var prev_is_yellow uint
var Blues uint

if last_is_yellow and if $color $color='blue':
    inc Blues 1
if $color $color='yellow':
    set prev_is_yellow 1
else:
    set prev_is_yellow 0

end:
    if prev_is_yellow and not if last_is_yellow:
        set last_is_yellow 1
        rewind
```
This example first checks the color of the last event. If it is yellow,
we go back to the beginning with `rewind` and count the number of blues.
After counting the blues, we need to skip the last event check in `end`,
otherwise we would be looping through the trail infinitely.

### Grouping by Splitting Contexts

All the examples seen this far produce one set of metrics, that is,
one row of output. This is similar to what you would get with a SQL
statement that only consists of aggregate functions, like `select
count(*), sum(field) from ... where ...`. Often you want to get metrics
for many groups separately, which is what you get with `group by` in
SQL.

With a declarative language like SQL, the query engine can figure out
the groupings automatically. Reel, being more imperative in nature,
requires a more explicit approach.

Reel programs manage their state in a **context** that contains all
variables of the program. Grouping in Reel is done by creating new
copies of the context on the fly with the `fork` statement. Each
context with its variables becomes a row of output.

Groups are identified by a key that can be any scalar variable. An
important feature of groupings is that by default each context, that is,
each key is activated only once for each trail. The following example
makes it clear why this is a good idea.

#### Example: [07-split.rl](/doc/07-split.rl)
Count the number of hedgehogs grouped by color.
```Go
var Color item
var Hedgehogs uint

if Color $color:
    # this is evaluated in the child context
    if $hedgehog:
        inc Hedgehogs 1
else:
    # this is evaluated in the parent context
    if $color:
        fork $color:
            send Color $color
```
Reel programs with groupings have typically two branches of execution:
The parent, which takes care of creating the desired groups with `fork`,
and the child contexts, or the groups, which compute the actual metrics.
We know which branch to execute based on the group label, like `Color`,
above - only the children have a non-empty group label.

The example above creates a new group, keyed by the `$color` field, for
each event seen with a non-empty `$color`. This is necessary since we do
not know the groups in advance. It would be wasteful to evaluate all
possible groups opportunistically. The fact that Reel deduplicates groups
based on their keys ensures that we compute metrics for each group only
once.

The actions under `fork` are evaluated only when the group is activated.
A special action, `send`, allows the parent to set variables in the new
child context. In this case, we set the `Color` variable of the child to
correspond to the `$color` that invoked the child.

The output of the program looks like follows:
```
Color,Hedgehogs
,0
yellow,4
blue,1
```
which is what you would expect based on the picture above. Reel outputs
each context as a row, including the parent context, which explains the
first row with an empty `Color`. Learn how this can be avoided in
the section "Formatting Output" below.

### Managing State across Multiple Trails

All the examples this far have computed metrics over a single trail. In
real life, it is more typical to compute metrics over multiple trails,
which Reel supports without trouble. You just need to manage state
properly, as shown below.

Let's move on to another example TrailDB, `/doc/hedgehog2.tdb`, which
contains another trail in addition to the one we have been using this far:

![Hedgehog Example 2](/doc/hedgehog2.png)

By default `reel` executes the program for every trail in the TrailDB. The
`begin` pattern is called in the beginning of each trail but otherwise the
program state is kept intact over trails.

You can see this by executing the hedgehog counter over the two-trail TrailDB:

`./reel doc/01-counter.rl doc/hedgehog2.tdb`

This should output 12, as there are 12 hedgehogs in total. You can also try

`./reel doc/02-colors.rl doc/hedgehog2.tdb`

which shows that there are 6 yellows and 6 blues in total.

However, try to execute the program that counts the number of blues for trails
that end with yellow color

`./reel doc/06-rewind.rl doc/hedgehog2.tdb`

This returns 6, which is not correct. The program counts the blues in
the second trail, although it doesn't end with a yellow event. This is
because its internal state is incorrectly carried over from the previous
trail to the next one. It is easy to fix:

#### Example: [08-rewind-fixed.rl](/doc/08-rewind-fixed.rl)
```Go
var last_is_yellow uint
var prev_is_yellow uint
var Blues uint

begin:
    unset last_is_yellow
    unset prev_is_yellow

if last_is_yellow and if $color $color='blue':
    inc Blues 1
if $color $color='yellow':
    set prev_is_yellow 1
else:
    set prev_is_yellow 0

end:
    if prev_is_yellow and not if last_is_yellow:
        set last_is_yellow 1
        rewind
```

We added a `begin` block that resets the internal state by calling
`unset` for the trail-specific variables. This fixes the program. Note
that we do not want to reset the `Blues` counter, which is supposed to
collect statistics over multiple trails.

You should always carefully initialize any variables in `begin` that are
not supposed to leak over trails.

### Formatting Output

You can post-process and output results of a Reel program arbitrarily by
using its API, as described in the "Embedding Reel" section below. This
section focuses on the Reel command line tool, which outputs CSV.

By convention, all variables starting with an uppercase letter are output
as columns in the CSV. The order of variable declarations defines the order
of the columns. Lookup tables starting with an uppercase letter are expanded
to columns using `variable:value` notation for the column labels.

Every context becomes a row in the CSV. This is not always desirable, as seen
in the group-by example above. Often, you want to exclude the parent context
from the output, or you may decide, based on the computed metrics, that a row
should not be included in the results. To make this possible, Reel defines a
special variable `_HIDE`, which if set to a non-zero value, causes the context
to be excluded from the output. Here is an example, extending the previous
group-by example:

#### Example: [09-hide.rl](/doc/09-hide.rl)

```Go
var Color item
var Hedgehogs uint
var _HIDE uint

begin:
    if Color:
        set _HIDE 0
    else:
        set _HIDE 1

if Color $color:
    # this is evaluated in the child context
    if $hedgehog:
        inc Hedgehogs 1
else:
    # this is evaluated in the parent context
    if $color:
        fork $color:
            send Color $color
```
This program excludes the parent context from the output using the `_HIDE` variable. Only
`yellow` and `blue` are output.

### Embedding Reel

A Reel program compiles to a self-sufficient C object. The objects are
namespaced so that you can include multiple programs in your application
without name clashes. You can also compile the Reel program to a shared
library and use it from other languages through their Foreign Function
Interface.

TODO: Document API, examples.

### Extending Reel

Reel is designed to be easily extensible with user-defined actions.
Since Reel is strongly typed, you need to provide type-specialized
versions of your actions, defined as C functions, for the types that
your action accepts.

TODO: Example.
