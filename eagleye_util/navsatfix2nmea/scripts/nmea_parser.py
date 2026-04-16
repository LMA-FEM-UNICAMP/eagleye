import math
from datetime import datetime

KNOT_TO_MPS = 0.514444

def compute_checksum(nmea_body):
    cs = 0
    for c in nmea_body:
        cs ^= ord(c)
    return f"{cs:02X}"

def parse_lat_lon(value, direction):
    if not value:
        return None
    if len(value) < 4:
        return None

    if direction in ['N', 'S']:
        deg = int(value[:2])
        minutes = float(value[2:])
    else:
        deg = int(value[:3])
        minutes = float(value[3:])

    decimal = deg + minutes / 60.0
    if direction in ['S', 'W']:
        decimal *= -1

    return decimal

def parse_time_date(timestr, datestr=None):
    if not timestr:
        return None
    try:
        hh = int(timestr[0:2])
        mm = int(timestr[2:4])
        ss = int(timestr[4:6])
        dt = datetime.utcnow().replace(hour=hh, minute=mm, second=ss)

        if datestr:
            dd = int(datestr[0:2])
            mo = int(datestr[2:4])
            yy = int(datestr[4:6]) + 2000
            dt = dt.replace(day=dd, month=mo, year=yy)

        return dt
    except:
        return None

def parse_rmc(fields):
    data = {}

    data["type"] = "RMC"
    data["time"] = parse_time_date(fields[1], fields[9])
    data["status"] = fields[2]

    lat = parse_lat_lon(fields[3], fields[4])
    lon = parse_lat_lon(fields[5], fields[6])

    speed_knots = float(fields[7]) if fields[7] else 0.0
    course_deg = float(fields[8]) if fields[8] else 0.0

    speed_mps = speed_knots * KNOT_TO_MPS

    # velocity components (ENU)
    psi = math.radians(course_deg)
    vx = speed_mps * math.sin(psi)
    vy = speed_mps * math.cos(psi)

    data.update({
        "latitude": lat,
        "longitude": lon,
        "speed_knots": speed_knots,
        "speed_mps": speed_mps,
        "course_deg": course_deg,
        "vx_east": vx,
        "vy_north": vy,
        "mode": fields[12] if len(fields) > 12 else None
    })

    return data

def parse_gga(fields):
    data = {}

    data["type"] = "GGA"
    data["time"] = parse_time_date(fields[1])

    lat = parse_lat_lon(fields[2], fields[3])
    lon = parse_lat_lon(fields[4], fields[5])

    fix_quality = int(fields[6]) if fields[6] else 0
    num_sat = int(fields[7]) if fields[7] else 0
    hdop = float(fields[8]) if fields[8] else None
    altitude = float(fields[9]) if fields[9] else None

    data.update({
        "latitude": lat,
        "longitude": lon,
        "fix_quality": fix_quality,
        "num_satellites": num_sat,
        "hdop": hdop,
        "altitude_m": altitude
    })

    return data

def parse_nmea(sentence):
    sentence = sentence.strip()

    if not sentence.startswith("$"):
        print("Invalid sentence (no $)")
        return

    if "*" in sentence:
        body, checksum = sentence[1:].split("*")
        calc = compute_checksum(body)
        valid_checksum = (calc.upper() == checksum.upper())
    else:
        body = sentence[1:]
        checksum = None
        valid_checksum = False
        calc = 0

    fields = body.split(",")
    msg_type = fields[0]

    print("\n--- RAW ---")
    print(sentence)

    print("\n--- CHECKSUM ---")
    print(f"Given: {checksum} | Computed: {calc} | Valid: {valid_checksum}")

    if msg_type.endswith("RMC"):
        data = parse_rmc(fields)
    elif msg_type.endswith("GGA"):
        data = parse_gga(fields)
    else:
        print(f"Unsupported message: {msg_type}")
        return

    print("\n--- DECODED ---")
    for k, v in data.items():
        print(f"{k:20}: {v}")

    print("\n--- SUMMARY ---")
    if data["type"] == "RMC":
        print(f"Position: ({data['latitude']}, {data['longitude']})")
        print(f"Speed: {data['speed_mps']:.2f} m/s ({data['speed_knots']} knots)")
        print(f"Velocity ENU: vx={data['vx_east']:.2f}, vy={data['vy_north']:.2f}")
    else:
        print(f"Fix quality: {data['fix_quality']}")
        print(f"Satellites: {data['num_satellites']}")
        print(f"Altitude: {data['altitude_m']} m")
        print(f"HDOP: {data['hdop']}")

def main():
    print("Enter NMEA sentences (Ctrl+D to exit):")
    try:
        while True:
            line = input()
            if line:
                parse_nmea(line)
    except EOFError:
        pass

if __name__ == "__main__":
    main()