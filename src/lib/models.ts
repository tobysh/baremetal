import type { ObjectId } from "mongodb";

export const EXPERIENCE_LEVELS = {
  none: "I've never written code",
  some: "I've done a little (tutorials, scripts)",
  other_lang: "I'm comfortable in another language (Python, JS…)",
  experienced: "I'm an experienced developer, new to low-level",
} as const;
export type ExperienceLevel = keyof typeof EXPERIENCE_LEVELS;

export type UserDoc = {
  _id: ObjectId;
  email: string;
  name: string;
  /** Public, lowercase, unique. Optional for accounts created before usernames existed. */
  username?: string;
  /** Total XP, kept in step with passed lessons (see progress.ts / scripts/recompute-xp.mts). */
  xp?: number;
  /** Whether the profile appears on /u/<username> and the leaderboard. */
  profilePublic?: boolean;
  /** IANA time zone name; weeks for the weekly rhythm start on Monday in this zone. Unset until first seen. */
  timezone?: string;
  /** Lessons per week the learner is aiming for; 0 means no goal. Unset means the default. */
  weeklyGoal?: number;
  /** New accounts are false until verified; absent on legacy accounts. */
  emailVerified?: boolean;
  passwordHash: string;
  experience: ExperienceLevel;
  role: "user" | "admin";
  createdAt: Date;
  termsAcceptedAt: Date;
  termsVersion: string;
  /** Bumped on password change / reset / 2FA change; older sessions become invalid. */
  sessionVersion: number;
  passwordChangedAt?: Date;
  twoFactor?: {
    enabled: boolean;
    /** Base32 TOTP secret, AES-256-GCM encrypted. */
    secretEnc: string;
    /** Keyed hashes of unused backup codes; legacy SHA-256 entries remain usable once. */
    backupCodeHashes: string[];
    /** Last accepted TOTP time step, to reject replayed codes. */
    lastStep: number;
    enabledAt?: Date;
  };
};

export type SessionDoc = {
  _id: ObjectId;
  tokenHash: string;
  userId: ObjectId;
  sessionVersion: number;
  createdAt: Date;
  expiresAt: Date;
  userAgent?: string;
};

export type ProgressDoc = {
  _id: ObjectId;
  userId: ObjectId;
  exerciseId: string;
  trackSlug: string;
  code: string;
  status: "started" | "passed";
  attempts: number;
  /** XP granted when the lesson was first passed. */
  xpAwarded?: number;
  /** A first pass whose XP decision was interrupted and needs reconciliation. */
  xpPending?: boolean;
  /** First pass happened faster than a person can learn (see PASS_PACE), so no XP was granted. */
  xpWithheld?: boolean;
  /** Why XP was withheld: too many passes too fast, the rolling daily cap, or solved faster than the difficulty's floor. Reviewed with scripts/flagged-accounts.mts. */
  xpFlag?: "pace" | "daily-cap" | "too-fast";
  /** When the learner first touched this exercise (absent on documents from before the field existed). */
  startedAt?: Date;
  /** Whether a save, hint or failed attempt established a prior start time. */
  startKnown?: boolean;
  updatedAt: Date;
  passedAt?: Date;
  /** Highest number of authored hints the learner has opened (recorded by the server). */
  hintsUsed?: number;
  /** Snapshots taken on first pass; they decide the Unaided seal and some achievements. */
  hintsUsedAtPass?: number;
  attemptsAtPass?: number;
  /** Passed without hints or the tutor, after at least one failed run (or on a hard lesson). Cosmetic. */
  unaided?: boolean;
};

/** Every XP grant. The unique (userId, source, ref) index is the "award once" guarantee. */
export type XpEventDoc = { _id: ObjectId; userId: ObjectId; source: "lesson" | "challenge"; ref: string; xp: number; at: Date };

export type AchievementDoc = { _id: ObjectId; userId: ObjectId; id: string; unlockedAt: Date };

/** Spaced review of a passed lesson. Reviews never award XP. */
export type ReviewDoc = { _id: ObjectId; userId: ObjectId; exerciseId: string; dueAt: Date; interval: number; lastAt: Date; count: number };

export type TutorMessage = { role: "user" | "assistant"; content: string; at: Date };

export type TutorThreadDoc = {
  _id: ObjectId;
  userId: ObjectId;
  lessonId: string;
  messages: TutorMessage[];
  updatedAt: Date;
};

export const TERMS_VERSION = "2026-10-27";
