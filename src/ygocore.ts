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
  setcode_high: number;
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

export interface OcgEngineApi {
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
  setResponse(duel: number, response: Buffer): void;

  /**
   * tick
   * @param duel duel id
   */
  process(duel: number): ProcessResult;
}

const core = require('../build/Release/ocgcore');

export class OcgEngine implements OcgEngineApi {
  constructor(private readonly core: any) { }

  initializeEngine(): void {
    return this.core.initializeEngine();
  }

  registerScript(name: string, content: string):void {
    return this.core.registerScript(name, content);
  }
  
  registerCard(card: CardData): void {
    return this.core.registerCard(card);
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

  newCard(duel: number, nc: NewCard): void {
    return this.core.newCard(duel, nc);
  }

  setResponse(duel: number, response: Buffer): void {
    const arrayBuffer = response.buffer.slice(0, response.byteLength);
    return this.core.setResponse(duel, arrayBuffer);
  }

  process(duel: number): ProcessResult {
    const result = this.core.process(duel);
    const buffer = result.messages as ArrayBuffer;
    const messages = Buffer.from(buffer);
    return { flags: result.flags as number, messages };
  }
}

export const engine = new OcgEngine(core);
export const raw = core;