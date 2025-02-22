<script lang="ts">
    import { EspUtils } from "../backend/EspUtils";
    import type { ConfigData } from "../types/ConfigData";
    let config: ConfigData = EspUtils.getBlank();
    EspUtils.config().subscribe(value=> config = value);
    const handleSubmit = (event: Event) => {
      event.preventDefault();
      const update: Partial<ConfigData> = {
          motorEnabled: config.motorEnabled,
          launchLedEnabled: config.launchLedEnabled,
          audioEnabled: config.audioEnabled,
          pwrLedEnabled: config.pwrLedEnabled,
          motorPin: config.motorPin
      };
      EspUtils.updateConfig(update);
    };
</script>
<form on:submit={handleSubmit} class="row g-3">
  <div class="col-4">
    <div class="input-group">
      <div class="input-group-text">
        <div class="form-check form-switch">
          <input class="form-check-input" type="checkbox" role="switch" id="enableLaunchLED" bind:checked={config.launchLedEnabled}/>
          <label class="form-check-label" for="enableLaunchLED">Launch LED</label>
        </div>
      </div>
    </div>
  </div>
  <div class="col-4">
    <div class="input-group">
      <div class="input-group-text">
        <div class="form-check form-switch">
          <input class="form-check-input" type="checkbox" role="switch" id="enablePwrLED" bind:checked={config.pwrLedEnabled}/>
          <label class="form-check-label" for="enablePwrLED">Power LED</label>
        </div>
      </div>
    </div>
  </div>
  <div class="col-4">
    <div class="input-group">
      <div class="input-group-text">
        <div class="form-check form-switch">
          <input class="form-check-input" type="checkbox" role="switch" id="enableAudio" bind:checked={config.audioEnabled}/>
          <label class="form-check-label" for="enableAudio">Audio Module</label>
        </div>
      </div>
    </div>
  </div>
  <div class="col-4">
    <div class="input-group">
      <div class="input-group-text">
        <div class="form-check form-switch">
          <input class="form-check-input" type="checkbox" role="switch" id="enableRumbleLED" bind:checked={config.motorEnabled}/>
          <label class="form-check-label" for="enableRumbleLED">Rumble Motor</label>
        </div>
      </div>
      <div class="form-floating">
        <input type="number" class="form-control" id="pinMotor" placeholder="32" bind:value={config.motorPin} disabled={!config.motorEnabled} />
        <label for="pinMotor">GPIO PIN</label>
      </div>
    </div>
  </div>
  <button type="submit" class="btn btn-primary mt-3">Save</button>
</form>

