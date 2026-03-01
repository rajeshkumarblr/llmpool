import base64
import requests
import json
import os

def analyze_image_with_ollama(image_path):
    # Check if file exists
    if not os.path.exists(image_path):
        print(f"Error: {image_path} not found")
        return

    # 1. Encode image to Base64
    with open(image_path, "rb") as image_file:
        base64_image = base64.b64encode(image_file.read()).decode('utf-8')

    # 2. Prepare the payload
    url = "http://localhost:11434/api/generate"
    payload = {
        "model": "qwen3-vl:latest",
        "prompt": "What is in this image?",
        "stream": False,
        "images": [base64_image]
    }

    # 3. Call Ollama API
    try:
        response = requests.post(url, json=payload)
        response.raise_for_status() # Raise error for bad status codes
        
        result = response.json()
        print(f"\nResults for '{image_path}':")
        print(result.get("response", "No response field in JSON"))
        
    except requests.exceptions.RequestException as e:
        print(f"Error calling Ollama API: {e}")

if __name__ == "__main__":
    # Example usage: analyze all images in the 'images' directory
    image_dir = "images"
    
    if os.path.exists(image_dir) and os.path.isdir(image_dir):
        print("Starting image analysis pipeline (Python)...")
        for filename in os.listdir(image_dir):
            if filename.lower().endswith(('.png', '.jpg', '.jpeg', '.webp')):
                path = os.path.join(image_dir, filename)
                analyze_image_with_ollama(path)
    else:
        print(f"Directory '{image_dir}' does not exist.")
