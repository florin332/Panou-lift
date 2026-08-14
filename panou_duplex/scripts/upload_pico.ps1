param(
    [Parameter(Mandatory = $true)]
    [string]$FirmwarePath,

    [Parameter(Mandatory = $true)]
    [string]$UploadPort
)

$ErrorActionPreference = "Stop"

function Get-PicoPorts {
    $ports = @(
        Get-CimInstance Win32_SerialPort |
            Where-Object { $_.PNPDeviceID -match "VID_2E8A" } |
            Select-Object -ExpandProperty DeviceID
    )

    $pnpPorts = Get-CimInstance Win32_PnPEntity |
        Where-Object { $_.PNPDeviceID -match "VID_2E8A" -and $_.Name -match "COM\d+" }

    foreach ($device in $pnpPorts) {
        if ($device.Name -match "(COM\d+)") {
            $ports += $Matches[1]
        }
    }

    return @($ports | Sort-Object -Unique)
}

$firmware = (Resolve-Path $FirmwarePath -ErrorAction SilentlyContinue).Path
if ([string]::IsNullOrWhiteSpace($firmware) -or -not (Test-Path -LiteralPath $firmware)) {
    throw "Firmware .uf2 not found: $FirmwarePath"
}

$projectConfig = Join-Path (Get-Location) "platformio.ini"
$configuredUploadPort = ""
if (Test-Path $projectConfig) {
    $configLine = Select-String -Path $projectConfig -Pattern '^\s*upload_port\s*=\s*(\S+)' |
        Select-Object -First 1
    if ($configLine -and $configLine.Matches[0].Groups[1].Value) {
        $configuredUploadPort = $configLine.Matches[0].Groups[1].Value.Trim()
    }
}

$autoSelection = $configuredUploadPort -eq "" -or $configuredUploadPort -ieq "auto"
$uploadPort = $UploadPort.Trim()
$initialBootloaderDrives = @(Get-CimInstance Win32_LogicalDisk |
    Where-Object { $_.VolumeName -eq "RPI-RP2" } |
    Select-Object -ExpandProperty DeviceID)

if ($initialBootloaderDrives.Count -gt 1) {
    throw "Multiple RPI-RP2 bootloaders are mounted; target Pico is ambiguous"
}

$picoPorts = @(Get-PicoPorts)
$picoUsbBoards = @(Get-CimInstance Win32_PnPEntity |
    Where-Object { $_.PNPDeviceID -match "VID_2E8A&PID_0003\\[^&]+$" })

if ($autoSelection) {
    if ($picoUsbBoards.Count -gt 1) {
        throw "Multiple Pico boards detected; set upload_port explicitly (available ports: $($picoPorts -join ', '))"
    }

    if ($picoPorts.Count -eq 0 -and $initialBootloaderDrives.Count -eq 0) {
        throw "No Raspberry Pi Pico serial port detected"
    }

    if ($picoPorts.Count -gt 1) {
        throw "Multiple Pico boards detected ($($picoPorts -join ', ')); set upload_port explicitly"
    }

    $serialPort = if ($picoPorts.Count -eq 1) { $picoPorts[0] } else { $null }
} else {
    if ($uploadPort -notmatch '^COM\d+$') {
        throw "Invalid upload_port '$UploadPort'; use auto or a COM port such as COM16"
    }

    if ($picoPorts -notcontains $uploadPort -and $initialBootloaderDrives.Count -eq 0) {
        throw "Selected port $uploadPort is not a detected Raspberry Pi Pico"
    }

    $serialPort = if ($picoPorts -contains $uploadPort) { $uploadPort } else { $null }
}

if ($autoSelection -and $initialBootloaderDrives.Count -eq 1) {
    $serialPort = $null
}

$bootloaderDrive = if ($initialBootloaderDrives.Count -eq 1) {
    $initialBootloaderDrives[0]
} else {
    $null
}

$reset = $false
for ($attempt = 0; $serialPort -and -not $reset -and $attempt -lt 3; $attempt++) {
    $serial = $null
    try {
        $serial = New-Object System.IO.Ports.SerialPort($serialPort, 1200, 'None', 8, 'One')
        $serial.Open()
        $serial.Close()
        $reset = $true
    } catch {
        if ($serial -ne $null) {
            $serial.Dispose()
        }
        Start-Sleep -Milliseconds 500
    }
}

if ($serialPort -and -not $reset) {
    $packagesPath = Join-Path $env:USERPROFILE ".platformio\packages"
    $picotool = Get-ChildItem -Path $packagesPath -Recurse -Filter "picotool.exe" -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match "tool-picotool-rp2040-earlephilhower" } |
        Sort-Object FullName -Descending |
        Select-Object -First 1 -ExpandProperty FullName

    if (-not $picotool) {
        $picotool = Get-ChildItem -Path $packagesPath -Recurse -Filter "picotool.exe" -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1 -ExpandProperty FullName
    }

    if (Test-Path $picotool) {
        & $picotool reboot -u -f --vid 0x2E8A --pid 0x0003 | Out-Null
        if ($LASTEXITCODE -eq 0) {
            $reset = $true
        }
    }

    if (-not $reset) {
        $bootloaderAfterReset = @(Get-CimInstance Win32_LogicalDisk |
            Where-Object { $_.VolumeName -eq "RPI-RP2" } |
            Select-Object -ExpandProperty DeviceID)

        if ($bootloaderAfterReset.Count -eq 1) {
            $bootloaderDrive = $bootloaderAfterReset[0]
            $reset = $true
        }
    }

    if (-not $reset) {
        throw "Could not reset Pico on $serialPort; close Serial Monitor and retry"
    }
}

Start-Sleep -Milliseconds 200

for ($attempt = 0; -not $bootloaderDrive -and $attempt -lt 40; $attempt++) {
    $currentBootloaderDrives = @(Get-CimInstance Win32_LogicalDisk |
        Where-Object { $_.VolumeName -eq "RPI-RP2" } |
        Select-Object -ExpandProperty DeviceID)

    $newBootloaderDrives = @($currentBootloaderDrives |
        Where-Object { $initialBootloaderDrives -notcontains $_ })

    if ($newBootloaderDrives.Count -eq 1) {
        $bootloaderDrive = $newBootloaderDrives[0]
    } elseif ($newBootloaderDrives.Count -gt 1) {
        throw "Multiple new RPI-RP2 bootloaders detected; target Pico is ambiguous"
    }

    if ($bootloaderDrive) { break }
    Start-Sleep -Milliseconds 250
}

if (-not $bootloaderDrive) {
    throw "RPI-RP2 bootloader volume not found after reset"
}

Copy-Item -Force $firmware ($bootloaderDrive + "\")

for ($attempt = 0; $attempt -lt 40; $attempt++) {
    $bootloaderStillMounted = Get-CimInstance Win32_LogicalDisk |
        Where-Object { $_.DeviceID -eq $bootloaderDrive -and $_.VolumeName -eq "RPI-RP2" }

    if (-not $bootloaderStillMounted) { break }
    Start-Sleep -Milliseconds 250
}

if ($bootloaderStillMounted) {
    throw "Firmware was copied, but Pico did not leave the RPI-RP2 bootloader"
}

for ($attempt = 0; $attempt -lt 40; $attempt++) {
    $picoSerial = @(Get-PicoPorts)

    if ($picoSerial) { break }
    Start-Sleep -Milliseconds 250
}

if (-not $picoSerial) {
    throw "Pico bootloader closed, but the firmware USB device did not reappear"
}

Write-Host "Uploaded $firmware to $bootloaderDrive and Pico restarted"