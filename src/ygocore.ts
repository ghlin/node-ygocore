import { POS, LOCATION, parseMessage } from './coremsg';

export interface CardData {
  code: number;
  alias: number;
  type: number;
  level: number;
  attribute: number;
  race: number;
  attack: number;
  defense: number;
  lscale: number;
  rscale: number;
  link_marker: number;

  /**
   * high 32-bits of setcode
   * split setcode into high/low to ensure
   * the value fits in a safe-integer
   *
   * @see Math.isSafeInteger
   */
  setcode_high: number;

  /**
   * low 32-bits of setcode
   * split setcode into high/low to ensure
   * the value fits in a safe-integer
   *
   * @see Math.isSafeInteger
   */
  setcode_low: number;
}

export interface PlayerInfo {
  /**
   * player id
   */
  player: number;

  /**
   * startup LP
   */
  lp: number;

  /**
   * initial hand count
   */
  start: number;

  /**
   * draw count (each turn)
   */
  draw: number;
}

export interface NewCard {
  code: number;
  owner: number;
  player: number;
  location: number;
  sequence: number;
  position: number;
}

export interface ProcessResult {
  messages: Buffer;
  flags: number;
}

export interface OcgcoreRaw {
  /**
   * initialize engine.
   *
   * must be called before any calls below.
   */
  initializeEngine(): void;

  /**
   * register static card data to engine.
   * @param card card data to register
   */
  registerCard(card: CardData): void;

  /**
   * register a script to engine.
   * @param name filename of the script (just the basename+ext)
   * @param content script content
   */
  registerScript(name: string, content: string): void;

  /**
   * create a duel
   *
   * returns the duel id, pass it to below functions.
   * @param seed random seed
   */
  createDuel(seed: number): number;

  /**
   * create a duel with seed from ygopro's replay
   *
   * returns the duel id, pass it to below functions.
   * @param seed random seed
   */
  createYgoproReplayDuel(seed: number): number;

  /**
   * start a duel
   * @param duel duel id
   * @param options duel options
   */
  startDuel(duel: number, options: number): void;

  /**
   * terminate a duel
   * @param duel duel id
   */
  endDuel(duel: number): void;

  /**
   * set player's initial info, see @see PlayerInfo
   * @param duel duel id
   * @param playerInfo player info to inform the engine
   */
  setPlayerInfo(duel: number, playerInfo: PlayerInfo): void;

  /**
   * add card to a duel, see @see NewCard
   * @param duel duel id
   * @param nc card info
   */
  newCard(duel: number, nc: NewCard): void;

  /**
   * write response to engine
   * @param duel duel id
   * @param response response
   */
  setResponse(duel: number, response: ArrayBuffer): void;

  /**
   * tick
   * @param duel duel id
   */
  process(duel: number): ProcessResult;
}

const core = require('../build/Release/ocgcore') as OcgcoreRaw;

export class OcgEngine {
  constructor(private readonly core: any) { }

  initializeEngine(): void {
    return this.core.initializeEngine();
  }

  registerScript(name: string, content: string): void {
    return this.core.registerScript(name, content);
  }

  registerCard(card: CardData): void {
    return this.core.registerCard(card);
  }

  createYgoproReplayDuel(seed: number) {
    return this.core.createYgoproReplayDuel(seed) as number;
  }

  createDuel(seed: number) {
    return this.core.createDuel(seed) as number;
  }

  startDuel(duel: number, options: number): void {
    return this.core.startDuel(duel, options);
  }

  endDuel(duel: number): void {
    return this.core.endDuel(duel);
  }

  setPlayerInfo(duel: number, playerInfo: PlayerInfo): void {
    return this.core.setPlayerInfo(duel, playerInfo);
  }

  newStartupCardMain(duel: number, player: number, code: number) {
    return this.newCard(duel, {
      player, owner: player, position: POS.FACEDOWN_DEFENSE,
      sequence: 0, location: LOCATION.DECK, code
    });
  }

  newStartupCardExtra(duel: number, player: number, code: number) {
    return this.newCard(duel, {
      player, owner: player, position: POS.FACEDOWN_DEFENSE,
      sequence: 0, location: LOCATION.EXTRA, code
    });
  }

  newCard(duel: number, nc: NewCard): void {
    return this.core.newCard(duel, nc);
  }

  setResponse(duel: number, response: Buffer): void {
    const arrayBuffer = response.buffer.slice(
      response.byteOffset,
      response.byteOffset + response.byteLength);
    return this.core.setResponse(duel, arrayBuffer);
  }

  process(duel: number) {
    const result = this.core.process(duel);
    const buffer = result.messages as ArrayBuffer;
    const messages = Buffer.from(buffer);
    return { flags: result.flags as number, messages: parseMessage(messages) };
  }
}

export const engine = new OcgEngine(core);
export const raw = core;
