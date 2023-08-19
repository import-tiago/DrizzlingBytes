Import("env")

# Define a custom post-upload action to run uploadfs
def run_uploadfs(source, target, env):
    print("Running uploadfs...")
    env.Execute("platformio run --target uploadfs")

# Attach the custom action to the post_upload hook
env.AddPostAction("upload", run_uploadfs)
