#define BUZZER_PIN 19

void playCoinSound()
{
  // Play coin sound
  tone(BUZZER_PIN, NOTE_B5, 50);
  delay(100);
  tone(BUZZER_PIN, NOTE_E6, 250);
  delay(300);
  noTone(BUZZER_PIN);
}

void playNext()
{
  //play next sound effect
  tone(BUZZER_PIN, NOTE_B5, 50);
  delay(800);
  noTone(BUZZER_PIN);
}

void playOk()
{
  //play ok sound effect
  tone(BUZZER_PIN,NOTE_G4,35);
  delay(50);
  tone(BUZZER_PIN,NOTE_G5,35);
  delay(50);
  tone(BUZZER_PIN,NOTE_G4,35);
  delay(50);
  tone(BUZZER_PIN,NOTE_G5,35);
  delay(50);
  tone(BUZZER_PIN,NOTE_G6,35);
  delay(500);
  noTone(BUZZER_PIN);
}

void playVictory()
{
  tone(BUZZER_PIN, 523.25, 133);
  delay(133);
  tone(BUZZER_PIN, 523.25, 133);
  delay(133);
  tone(BUZZER_PIN, 523.25, 133);
  delay(133);
  tone(BUZZER_PIN, 523.25, 400);
  delay(400);
  tone(BUZZER_PIN, 415.30, 400);
  delay(400);
  tone(BUZZER_PIN, 466.16, 400);
  delay(400);
  tone(BUZZER_PIN, 523.25, 133);
  delay(133);
  delay(133);
  tone(BUZZER_PIN, 466.16, 133);
  delay(133);
  tone(BUZZER_PIN, 523.25, 1200);
  delay(1200);
}

void playLose() 
{
  tone(BUZZER_PIN, NOTE_C5, 133);
  delay(133);
  tone(BUZZER_PIN, NOTE_G4, 133);
  delay(133);
  tone(BUZZER_PIN, NOTE_E4, 400);
  delay(400);
  tone(BUZZER_PIN, NOTE_A4, 266);
  delay(266);
  tone(BUZZER_PIN, NOTE_B4, 266);
  delay(266);
  tone(BUZZER_PIN, NOTE_A4, 266);
  delay(266);
  tone(BUZZER_PIN, NOTE_GS4, 266);
  delay(266);
  tone(BUZZER_PIN, NOTE_AS4, 266);
  delay(266);
  tone(BUZZER_PIN, NOTE_GS4, 266);
  delay(266);
  tone(BUZZER_PIN, NOTE_G4, 800);
  delay(800);
  tone(BUZZER_PIN, NOTE_D4, 800);
  delay(800);
  tone(BUZZER_PIN, NOTE_E4, 400);
  delay(200);
}