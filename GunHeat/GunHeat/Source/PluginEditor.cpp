#include "PluginEditor.h"

HeatEditor::HeatEditor (HeatProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setSize (500, 340);

    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour (juce::Slider::backgroundColourId, juce::Colour (0xff22252a));
    addAndMakeVisible (slider);
    attach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, "heat", slider);

    startTimerHz (30);
}

void HeatEditor::resized() { slider.setBounds (40, 295, 420, 28); }

juce::Colour HeatEditor::heatColour (float h)
{
    using juce::Colour;
    const Colour c0 (0xff3a3f46), c1 (0xff7a2a1c), c2 (0xffd8421a), c3 (0xffff9a1f), c4 (0xffffeaa8);
    h = juce::jlimit (0.0f, 1.0f, h);
    if (h < 0.35f) return c0.interpolatedWith (c1, h / 0.35f);
    if (h < 0.65f) return c1.interpolatedWith (c2, (h - 0.35f) / 0.30f);
    if (h < 0.90f) return c2.interpolatedWith (c3, (h - 0.65f) / 0.25f);
    return c3.interpolatedWith (c4, (h - 0.90f) / 0.10f);
}

void HeatEditor::timerCallback()
{
    const float target = (float) slider.getValue() / 100.0f;
    heat += (target - heat) * (target > heat ? 0.12f : 0.04f);   // ısınma hızlı, soğuma yavaş
    time += 0.033f;

    const float mx = gunX + 280.0f * gunScale;
    const float my = gunY + 38.0f * gunScale;

    if (heat > 0.25f && rnd.nextFloat() < heat * 0.6f)   // duman
        particles.push_back ({ mx + rnd.nextFloat() * 8 - 4, my - 6, rnd.nextFloat() * 0.8f - 0.2f,
                               -(0.6f + rnd.nextFloat() * 0.9f), 1.0f, 6 + rnd.nextFloat() * 10, false });

    if (heat > 0.6f && rnd.nextFloat() < (heat - 0.6f) * 1.5f)   // kıvılcım
        particles.push_back ({ mx, my, 1.0f + rnd.nextFloat() * 3.0f,
                               -1.5f + rnd.nextFloat() * 2.5f, 1.0f, 3.0f, true });

    for (auto& p : particles)
    {
        p.x += p.vx; p.y += p.vy;
        if (p.spark) { p.vy += 0.15f; p.life -= 0.03f; }
        else         { p.life -= 0.015f; }
    }
    particles.erase (std::remove_if (particles.begin(), particles.end(),
                     [] (const Particle& p) { return p.life <= 0.0f; }), particles.end());

    slider.setColour (juce::Slider::thumbColourId, heatColour (heat).brighter (0.2f));
    slider.setColour (juce::Slider::trackColourId, heatColour (heat));
    repaint();
}

void HeatEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff15171b).interpolatedWith (heatColour (heat), heat * 0.18f),
                                             b.getCentreX(), 0.0f, juce::Colour (0xff0b0c0e),
                                             b.getCentreX(), b.getBottom(), false));
    g.fillAll();

    // arka plan parıltısı
    g.setGradientFill (juce::ColourGradient (heatColour (heat).withAlpha (heat * 0.55f), 250.0f, 120.0f,
                                             juce::Colours::transparentBlack, 250.0f, 340.0f, true));
    g.fillRect (b);

    // ekran titremesi
    const float sx = (rnd.nextFloat() - 0.5f) * 2.0f * heat * heat * 3.0f;
    const float sy = (rnd.nextFloat() - 0.5f) * 2.0f * heat * heat * 3.0f;
    const auto t = juce::AffineTransform::scale (gunScale).translated (gunX + sx, gunY + sy);

    // silah parçaları (300x150 lokal koordinat, namlu sağa bakıyor)
    juce::Path slide, frame, grip, guard, trigger, sightR, sightF;
    slide.addRoundedRectangle (30.0f, 20.0f, 250.0f, 36.0f, 6.0f);
    frame.addRoundedRectangle (30.0f, 56.0f, 170.0f, 16.0f, 4.0f);
    grip.startNewSubPath (70, 72); grip.lineTo (135, 72); grip.lineTo (112, 140); grip.lineTo (52, 140); grip.closeSubPath();
    guard.startNewSubPath (145, 72); guard.cubicTo (145, 104, 188, 104, 188, 72);
    trigger.addRectangle (162.0f, 72.0f, 5.0f, 16.0f);
    sightR.addRectangle (38.0f, 13.0f, 14.0f, 7.0f);
    sightF.addRectangle (265.0f, 13.0f, 8.0f, 7.0f);

    const auto hot  = heatColour (heat);
    const auto cool = heatColour (heat * 0.45f);

    g.setGradientFill (juce::ColourGradient (hot.brighter (0.25f), 0, gunY + 20 * gunScale,
                                             hot.darker (0.35f), 0, gunY + 56 * gunScale, false));
    g.fillPath (slide, t);
    g.setColour (hot.darker (0.2f));   g.fillPath (frame, t);  g.fillPath (sightR, t);  g.fillPath (sightF, t);
    g.setColour (cool.darker (0.3f));  g.fillPath (grip, t);   g.fillPath (trigger, t);
    g.setColour (hot.darker (0.4f));   g.strokePath (guard, juce::PathStrokeType (5.0f), t);

    // sürgü tırtıkları
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    for (int i = 0; i < 6; ++i)
    {
        juce::Path s; s.addRectangle (60.0f + i * 7.0f, 24.0f, 2.5f, 28.0f);
        g.fillPath (s, t);
    }
    // namlu ucu: iyice kızarsa beyaza döner
    juce::Path muzzle; muzzle.addRoundedRectangle (272.0f, 28.0f, 12.0f, 20.0f, 3.0f);
    g.setColour (heatColour (juce::jmin (1.0f, heat * 1.15f + 0.05f)).brighter (0.2f));
    g.fillPath (muzzle, t);

    // ısı dalgalanması (heat shimmer)
    if (heat > 0.2f)
        for (int i = 0; i < 3; ++i)
        {
            juce::Path w;
            for (float x = gunX; x <= gunX + 400.0f; x += 8.0f)
            {
                const float y = gunY - 6.0f - i * 9.0f + std::sin (x * 0.05f + time * 6.0f + i) * 3.0f * heat;
                if (x == gunX) w.startNewSubPath (x, y); else w.lineTo (x, y);
            }
            g.setColour (heatColour (heat).brighter (0.4f).withAlpha (heat * 0.22f));
            g.strokePath (w, juce::PathStrokeType (1.5f));
        }

    // parçacıklar
    for (auto& p : particles)
    {
        if (p.spark)
        {
            g.setColour (juce::Colour (0xffffb43c).withAlpha (p.life));
            g.fillEllipse (p.x, p.y, p.size, p.size);
        }
        else
        {
            const float s = p.size * (2.0f - p.life);
            g.setColour (juce::Colour (0xff9a9a9a).withAlpha (p.life * 0.22f));
            g.fillEllipse (p.x - s * 0.5f, p.y - s * 0.5f, s, s);
        }
    }

    // yazılar
    g.setColour (juce::Colour (0xff8a8f98));
    g.setFont (juce::Font (13.0f, juce::Font::bold));
    g.drawText ("GUN HEAT  -  BARREL DRIVE", 20, 10, 300, 20, juce::Justification::left);

    g.setColour (hot.brighter (0.3f));
    g.setFont (juce::Font (36.0f, juce::Font::bold));
    g.drawText (juce::String (juce::roundToInt (slider.getValue())) + "%", 300, 245, 160, 44, juce::Justification::right);
}
