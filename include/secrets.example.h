#pragma once

// ==============================================================================
// ORBIT v2 — Secrets & API Keys Template (Example)
// Copy this file to include/secrets.h and fill in your real credentials.
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. Wi-Fi Station Credentials
// ------------------------------------------------------------------------------
#define WIFI_SSID               "YourWiFiSSID"
#define WIFI_PASS               "YourWiFiPassword"

// ------------------------------------------------------------------------------
// 2. Voice & Speech-to-Text (STT) APIs
// ------------------------------------------------------------------------------
// OpenAI Whisper API key for transcribing voice memos recorded on ORBIT:
#define OPENAI_API_KEY          "sk-proj-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// ElevenLabs API key for ultra-realistic voice synthesis / TTS (optional):
#define ELEVENLABS_API_KEY      "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// ------------------------------------------------------------------------------
// 3. Ambient Intelligence (AI) Large Language Models
// ------------------------------------------------------------------------------
// Google Gemini API key for Gemini 1.5 Flash / Pro (Ask AI studio):
#define GEMINI_API_KEY          "AIzaSyxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// Anthropic Claude API key for Claude 3.5 Sonnet / Haiku (optional):
#define ANTHROPIC_API_KEY       "sk-ant-xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// Default AI Model Selection: "gemini", "openai", or "orbit"
#define DEFAULT_AI_MODEL        "gemini"

// ------------------------------------------------------------------------------
// 4. Environmental & Space Telemetry APIs
// ------------------------------------------------------------------------------
// OpenWeatherMap API key for live outdoor weather & climate sync:
#define OPENWEATHER_API_KEY     "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"

// Open-Notify Realtime ISS Space Station Tracking Endpoint:
#define ISS_TRACKER_API_URL     "http://api.open-notify.org/iss-now.json"

// ------------------------------------------------------------------------------
// 5. Cloud Database & Audio Storage (Supabase — 100% Free, No Credit Card)
// ------------------------------------------------------------------------------
// Supabase Project URL (from Supabase Dashboard -> Project Settings -> API)
#define SUPABASE_URL            "https://your-project.supabase.co"

// Supabase Public Anon Key (starts with ey...)
#define SUPABASE_ANON_KEY       "your-anon-key-here"

// Supabase Storage Bucket for audio memos (e.g. "orbit-recordings")
#define SUPABASE_STORAGE_BUCKET "orbit-recordings"

// Supabase Postgres Table name (e.g. "recordings")
#define SUPABASE_TABLE          "recordings"

// ------------------------------------------------------------------------------
// 6. Firebase Cloud Database & Storage (Optional Alternative)
// ------------------------------------------------------------------------------
#define FIREBASE_PROJECT_ID     "your-firebase-project"
#define FIREBASE_API_KEY        "AIzaSyxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
#define FIREBASE_STORAGE_BUCKET "your-firebase-project.firebasestorage.app"
#define FIREBASE_COLLECTION     "orbit_recordings"
#define FIREBASE_RTDB_URL       "https://your-firebase-project-default-rtdb.firebaseio.com"

