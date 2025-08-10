#!/usr/bin/env python3
"""
Control script for the Disaster Party Mock Server
"""

import requests
import sys
import argparse
import json

def send_command(base_url, command):
    """Send a command to the mock server"""
    try:
        if command == 'status':
            response = requests.get(f"{base_url}/_control/status", timeout=5)
        else:
            response = requests.post(f"{base_url}/_control/{command}", timeout=5)
        
        if response.status_code == 200:
            data = response.json()
            print(json.dumps(data, indent=2))
            return True
        else:
            print(f"Error: HTTP {response.status_code}")
            print(response.text)
            return False
    except requests.exceptions.ConnectionError:
        print(f"Error: Could not connect to server at {base_url}")
        return False
    except requests.exceptions.Timeout:
        print("Error: Request timed out")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='Control the Disaster Party Mock Server')
    parser.add_argument('command', choices=['status', 'restart', 'shutdown'], 
                       help='Command to send to the server')
    parser.add_argument('--url', default='http://localhost:8080', 
                       help='Base URL of the mock server (default: http://localhost:8080)')
    
    args = parser.parse_args()
    
    success = send_command(args.url, args.command)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()