const { ElevenLabsClient } = require("@elevenlabs/elevenlabs-js");

// ElevenLabs Client instance — automatically loads ELEVENLABS_API_KEY from environment
const elevenlabs = new ElevenLabsClient();

module.exports = { elevenlabs };

