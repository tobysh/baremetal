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
    /** SHA-256 hashes of unused backup codes. */
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
  updatedAt: Date;
  passedAt?: Date;
};

export type TutorMessage = { role: "user" | "assistant"; content: string; at: Date };

export type TutorThreadDoc = {
  _id: ObjectId;
  userId: ObjectId;
  lessonId: string;
  messages: TutorMessage[];
  updatedAt: Date;
};

export const TERMS_VERSION = "2026-10-27";
