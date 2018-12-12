# node-ygocore

**WIP** node bindings for [ygopro-core](https://github.com/moecube/ygopro-core) (the OCG script engine)

**NOTE** This is not Ygopro the game!!!

# Install

```
npm install ygocore
```

# How to use

To use this package, you need to know how `ocgcore` works.

> **Note**
>
> I'm not the author/contributer of `ygopro`, if my understandings on `ocgcore`'s
> API are not correct, please fire me an issue. Thanks!


``` typescript
import { engine } from 'ygopro';
```

> this project is writen in Typescript, each method
> is type-annotated.

### Prepare a duel

#### create a duel instance

> ocgapi: `create_duel()`

``` typescript
const duel = engine.createDuel(/* any random seed */ 0);
```

#### set duelist's LP/cards to draw

> ocgapi: `set_player_info()`

``` typescript
engine.setPlayerInfo(duel, { /* ... */ });
```

#### add card to the field

> ocgapi: `new_card()`

``` typescript
engine.newCard(duel, { /* ... */ });
```

### Duel!

#### start the duel

> ocgapi: `start_duel()`

``` typescript
engine.startDuel(duel, options);
```

#### process for one step

> ocgapi: `process()`, `get_message()`

``` typescript
const { flags, data } = engine.process(duel);
```

`data` is a buffer (of type `Buffer`) contains messages returned from `ocgcore`, to deserialize it, you can use [ygocore-interface](`https://github.com/ghlin/node-ygocore-interface`)'s `parseMessage` (see below)

#### write player's response

> ocgapi: `set_responsei()`, `set_responseb()`

``` typescript
engine.setResponse(duel, response);
```

### End the duel

> ocgapi: `end_duel()`

``` typescript
engine.endDuel(duel);
```

### Query the game field

#### query cards by location

> ocgapi: `query_field_card`

``` typescript
const data = engine.queryFieldCard(duel, { /* player, location, ... */ });
```
> Again, to deserialize the message, you can use [ygocore-interface](https://github.com/ghlin/node-ygocore-interface)'s `parseFieldCardQueryResult`.

#### query single card

> ocgapi: `query_card`

``` typescript
const data = engine.queryCard(duel, { /* player, location, sequence, ... */ });
```

> Again and again, to deserialize the message, you can use [ygocore-interface](https://github.com/ghlin/node-ygocore-interface)'s `parseCardQueryResult`.

#### query card count

> ocgapi: `query_field_count`

``` typescript
const howManyCards = engine.queryFieldCount(duel, { /* ... */ });
```

### ygocore-interface

``` typescript
const { parseMessage } from 'ygocore-interface';

const messages = parseMessage(data /* from engine.process */);

for (const message of messages) {
  if (message.msgtype == 'MSG_SELECT_IDLECMD') {
    // message is not of type 'MsgSelectIdlecmd'
    // handle MsgSelectIdlecmd here.

    // refresh M zone:
    const mzone = parseFieldCardQueryResult(engine.queryFieldCard(duel, {
      player, location, queryFlags, useCache
    });

    for (const card of mzone) {
      // play with card...
      // type card.
      //          ^
      //          |
      //      nice code completion!
    }
  }

  // handle other messages here.
}
```

Constants in `common.h` are also exported:

``` typescript
import { LOCATION, QUERY } from 'ygocore-interface';

/* .... */

const location = LOCATION.HAND;
const queryFlags = /* ... + ... + */ QUERY.LSCALE + QUERY.RSCALE + QUERY.STATUS;
const hand = parseFieldCardQueryResult(engine.queryFieldCard(duel, {
  player, location, queryFlags, useCache
});

```
