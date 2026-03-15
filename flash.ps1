# Set up ESP-IDF environment and run idf.py with all provided arguments.
# Reads WiFi credentials from secrets.env and writes them into sdkconfig.
# Usage: .\flash.ps1 build
#        .\flash.ps1 -p COM3 build flash monitor

$env:IDF_PATH = "C:\esp\v5.5.3\esp-idf"
$env:IDF_TOOLS_PATH = "C:\Espressif\tools"
$env:IDF_PYTHON_ENV_PATH = "C:\Espressif\tools\python\v5.5.3\venv"
$env:ESP_ROM_ELF_DIR = "C:\Espressif\tools\esp-rom-elfs\20241011"

$env:PATH = @(
    "C:\Espressif\tools\cmake\3.30.2\bin",
    "C:\Espressif\tools\ninja\1.12.1",
    "C:\Espressif\tools\idf-exe\1.0.3",
    "C:\Espressif\tools\xtensa-esp-elf\esp-14.2.0_20251107\xtensa-esp-elf\bin",
    "C:\Espressif\tools\python\v5.5.3\venv\Scripts",
    $env:PATH
) -join ";"

# Inject WiFi credentials from secrets.env into sdkconfig
$secretsFile = Join-Path $PSScriptRoot "secrets.env"
if (-not (Test-Path $secretsFile)) {
    Write-Error "secrets.env not found -- copy the template and fill in your credentials"
    exit 1
}

$secrets = @{}
Get-Content $secretsFile | Where-Object { $_ -match "^\s*\w" } | ForEach-Object {
    $key, $value = $_ -split "=", 2
    $secrets[$key.Trim()] = $value.Trim()
}

$sdkconfig = Join-Path $PSScriptRoot "sdkconfig"
if (Test-Path $sdkconfig) {
    $content = Get-Content $sdkconfig
    $replacements = @{
        'CONFIG_WIFI_SSID'     = $secrets['WIFI_SSID']
        'CONFIG_WIFI_PASSWORD' = $secrets['WIFI_PASSWORD']
        'CONFIG_UPLOAD_SECRET' = $secrets['UPLOAD_SECRET']
    }

    foreach ($key in $replacements.Keys) {
        $val = $replacements[$key]
        if ($content -match "$key=") {
            $content = $content -replace "$key=`".*`"", "$key=`"$val`""
        } else {
            $content += "$key=`"$val`""
        }
    }

    $content | Set-Content $sdkconfig
} else {
    # sdkconfig does not exist yet -- write minimal seed so idf.py picks up credentials on first build
    @(
        "CONFIG_WIFI_SSID=`"$($secrets['WIFI_SSID'])`"",
        "CONFIG_WIFI_PASSWORD=`"$($secrets['WIFI_PASSWORD'])`"",
        "CONFIG_UPLOAD_SECRET=`"$($secrets['UPLOAD_SECRET'])`""
    ) | Set-Content $sdkconfig
}

& C:\Espressif\tools\python\v5.5.3\venv\Scripts\python.exe `
    C:\esp\v5.5.3\esp-idf\tools\idf.py @args
