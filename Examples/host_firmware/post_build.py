Import("env")

# Define a custom post-build action to run buildfs
def run_buildfs(source, target, env):
    print("Running buildfs...")
    env.Execute("platformio run --target buildfs")

# Attach the custom action to the post_build hook
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", run_buildfs)